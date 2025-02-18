#include "dluau.hpp"
#include <common.hpp>
#include <chrono>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <utility>
#include <ranges>
#include <algorithm>
using namespace std::chrono_literals;
using namespace std::string_view_literals;
namespace rngs = std::ranges;
namespace vws = std::views;
namespace chrn = std::chrono;
using chrn::steady_clock;
using boost::container::flat_set;
using boost::container::flat_map;
using common::Raii, std::string_view;
using std::vector, std::string;
using std::expected, std::unexpected;
using chrn::duration, std::tuple;

struct Deferrer {
    lua_State* state;
    int ref;
    int argn;
};

struct Afterer {
    lua_State* after_this;
    int after_this_ref;
    int ref;
    int argn;
};

struct Waiter {
    steady_clock::time_point start;
    duration<double> length;
    int argn;
    int ref;
};
enum class Job {
    waiter, after, ctask
};
struct Cleaning_instruction {
    uintptr_t id;
    Job job;
};

static flat_map<lua_State*, Waiter> waiting;
static flat_map<lua_State*, Afterer> do_after;
static vector<dluau_CTask> ctasks;
static vector<Deferrer> deferred;
static vector<Cleaning_instruction> janitor;

static auto is_finished(lua_State* L, int ref) -> bool {
    lua_getglobal(L, "coroutine");
    lua_getfield(L, -1, "status");
    lua_remove(L, -2);
    lua_getref(L, ref);
    lua_call(L, 1, 1);
    const bool finished = lua_tostring(L, -1) == "dead"sv;
    lua_pop(L, 1);
    return finished;
}
static auto close_thread(lua_State* L, lua_State* co) -> expected<void, string> {
    lua_getglobal(L, "coroutine");
    lua_getfield(L, -1, "close");
    lua_remove(L, -2);
    lua_pushthread(co);
    lua_xmove(co, L, 1);
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        string errmsg = lua_tostring(L, -1);
        lua_pop(L, 1);
        return unexpected(std::move(errmsg));
    }
    return expected<void, string>{};
}

static auto has_errored(lua_State* state) -> bool {
    const int status = lua_status(state);
    return status != LUA_OK and status != LUA_YIELD;
}
static auto has_errored(int status) -> bool {
    return status != LUA_OK and status != LUA_YIELD;
}
static auto wait(lua_State* L) -> int {
    if (not lua_isyieldable(L)) luaL_errorL(L, "current context is not yieldable");
    const auto now = steady_clock::now(); 
    const auto sec = duration<double>(luaL_optnumber(L, 1, 0));
    if (waiting.contains(L)) {
        waiting[L].start = now;
        waiting[L].length = sec;
        return lua_yield(L, 0);
    }
    lua_pushthread(L);
    int ref = lua_ref(L, -1);
    waiting.emplace(L, Waiter{
        .start = now,
        .length = sec,
        .argn = 0,
        .ref = ref,
    });
    return lua_yield(L, 0);
}
static auto resolve_task_argument(lua_State* L, int idx) -> tuple<lua_State*, int> {
    lua_State* state = nullptr;
    const int top = lua_gettop(L);
    if (lua_isfunction(L, idx)) {
        state = lua_newthread(L);
        lua_pushvalue(L, idx);
        lua_xmove(L, state, 1);
        luaL_sandboxthread(state);
    } else if (lua_isthread(L, idx)) {
        state = lua_tothread(L, idx);
        lua_pushvalue(L, idx);
    }
    if (state) {
        for (int i{idx + 1}; i <= top; ++i) lua_pushvalue(L, i);
        lua_xmove(L, state, top - idx);
    }
    return {state, top - idx};
}
static auto spawn(lua_State* L) -> int {
    auto [state, argn] = resolve_task_argument(L, 1);
    if (not state) luaL_typeerrorL(L, 1, "function or thread");
    const int status = lua_resume(state, L, argn);
    if (has_errored(status)) {
        lua_xmove(state, L, 1);
        lua_error(L);
    }
    return 1;
}
static auto delay(lua_State* L) -> int {
    const auto now = steady_clock::now();
    const auto sec = duration<double>(luaL_optnumber(L, 1, 0));
    auto [state, argn] = resolve_task_argument(L, 2);
    if (not state) luaL_typeerrorL(L, 1, "function or thread");
    waiting.emplace(state, Waiter{
        .start = now,
        .length = sec,
        .argn = argn,
        .ref = lua_ref(L, -1),
    });
    return 1;
}
static auto defer(lua_State* L) -> int {
    auto [state, argn] = resolve_task_argument(L, 1);
    if (not state) luaL_typeerrorL(L, 1, "function or thread");
    deferred.push_back(Deferrer{
        .state = state,
        .ref = lua_ref(L, -1),
        .argn = argn,
    });
    return 1;
}
static auto cancel(lua_State* L) -> int {
    if (not lua_isthread(L, 1)) luaL_typeerrorL(L, 1, "thread");
    lua_State* state = lua_tothread(L, 1);
    if (auto success = close_thread(L, state); !success) dluau::error(L, success.error());
    if (waiting.contains(state)) {
        lua_unref(L, waiting[state].ref);
        waiting.erase(state);
        return 0;
    }
    auto in_deferred = rngs::find_if(deferred, [&state](Deferrer& e) {
        return e.state == state;
    });
    if (in_deferred != rngs::end(deferred)) {
        lua_unref(L, in_deferred->ref);
        deferred.erase(in_deferred);
        return 0;
    }
    return 0;
}
static auto index(lua_State* L) -> int {
    string_view key = luaL_checkstring(L, 2);
    if (key == "this") {
        lua_pushthread(L);
        return 1;
    }
    luaL_errorL(L, "unknown field '%s' in task", luaL_checkstring(L, 2));
}

static auto wait_util(lua_State* L) -> int {
    lua_State* thread = lua_tothread(L, 1);
    const int waiting_for_ref = lua_ref(L, 1);
    lua_pushthread(L);
    const int ref = lua_ref(L, -1);
    do_after.emplace(L, Afterer{
        .after_this = thread,
        .after_this_ref = waiting_for_ref,
        .ref = lua_ref(L, -1),
        .argn = 0,
    });
    return lua_yield(L, 1);
}
static auto delay_until(lua_State* L) -> int {
    lua_State* thread = lua_tothread(L, 1);
    const int waiting_for_ref = lua_ref(L, 1);
    auto [state, argn] = resolve_task_argument(L, 2);
    if (not state) luaL_typeerrorL(L, 2, "function or thread");
    do_after.emplace(state, Afterer{
        .after_this = thread,
        .after_this_ref = waiting_for_ref,
        .ref = lua_ref(L, -1),
        .argn = argn,
    });
    return 1;
}

namespace dluau {
auto tasks_in_progress() -> bool {
    return not (
        waiting.empty()
        and deferred.empty()
        and do_after.empty()
        and ctasks.empty()
    );
}
auto task_step(lua_State* L) -> expected<void, string> {
    const auto now = steady_clock::now();
    for (auto& [state, w] : do_after) {
        const int wf_top = lua_gettop(w.after_this);
        const int wf_status = lua_status(w.after_this);
        const bool finished = wf_status == LUA_OK and wf_top == 0;
        if (wf_status == LUA_YIELD or (not finished)) continue;
        if (wf_status == LUA_OK) {
            const int status = lua_resume(state, L, w.argn);
            if (has_errored(status)) {
                return unexpected(lua_tostring(state, -1));
            }
        }
        janitor.emplace_back(reinterpret_cast<uintptr_t>(state), Job::after);
    }
    for (auto& d : deferred | vws::reverse) {
        auto status = lua_resume(d.state, L, d.argn);
        lua_unref(L, d.ref);
        if (has_errored(status)) {
            return unexpected(lua_tostring(d.state, -1));
        }
    }
    deferred.clear();
    for (auto& [state, waiter] : waiting) {
        const auto old_start = waiter.start;
        if (now - waiter.start < waiter.length) continue;
        const int status = lua_resume(state, L, waiter.argn);
        const bool yielded_independently{
            status == LUA_YIELD
            and waiter.start == old_start
        };
        if (status == LUA_OK or yielded_independently) {
            janitor.emplace_back(reinterpret_cast<uintptr_t>(state), Job::waiter);
            continue;
        } else if (status != LUA_YIELD){
            return unexpected(lua_tostring(state, -1));
        }
    }
    for (dluau_CTask& task : ctasks) {
        const char* errmsg{nullptr};
        dluau_CTaskStatus status = task(&errmsg);
        switch (status) {
            case DLUAU_CTASK_CONTINUE:
                continue;
            case DLUAU_CTASK_DONE:
                janitor.emplace_back(reinterpret_cast<uintptr_t>(task), Job::ctask);
                continue;
            case DLUAU_CTASK_ERROR:
                return unexpected(errmsg ? errmsg : "external process failed");
        }
    }
    for (const auto& v : janitor) {
        switch (v.job) {
            case Job::waiter: {
                auto state = reinterpret_cast<lua_State*>(v.id);
                lua_unref(L, waiting.at(state).ref);
                waiting.erase(state);
                continue;
            } case Job::after: {
                auto state = reinterpret_cast<lua_State*>(v.id);
                const auto& w = do_after.at(state);
                lua_unref(L, w.after_this_ref);
                lua_unref(L, w.ref);
                do_after.erase(state);
                continue;
            } case Job::ctask: {
                auto task = reinterpret_cast<dluau_CTask>(v.id);
                auto found = rngs::find(ctasks, task);
                if (found == rngs::end(ctasks)) {
                    return unexpected(std::format("couldn't find ctask ({}) during the removal phase", v.id));
                }
                ctasks.erase(found);
                continue;
            }

        }
    }
    janitor.clear();
    return expected<void, string>();
}
}

void dluau_addctask(dluau_CTask cb) {
    ctasks.push_back(cb);
}
auto dluau_tasksinprogress() -> bool {
    return dluau::tasks_in_progress();
}
auto dluau_taskstep(lua_State* L) -> bool {
    auto r = dluau::task_step(L);
    if (not r) {
        lua_pushstring(L, r.error().c_str());
        return false;
    }
    return true;
}

void dluauopen_task(lua_State* L) {
    const luaL_Reg lib[] = {
        {"wait", wait},
        {"spawn", spawn},
        {"defer", defer},
        {"delay", delay},
        {"cancel", cancel},
        {"waituntil", wait_util},
        {"delayuntil", delay_until},
        {nullptr, nullptr}
    };
    lua_newtable(L);
    luaL_register(L, nullptr, lib);
    lua_newtable(L);
    lua_pushstring(L, "locked");
    lua_setfield(L, -2, "__metatable");
    lua_pushcfunction(L, index, "__index");
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
    lua_setreadonly(L, -1, true);
    lua_setglobal(L, "task");
}
