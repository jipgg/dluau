#pragma once
#ifdef DLUAU_LIB_EXPORT
#define DLUAU_API extern "C" __declspec(dllexport)
#else
#define DLUAU_API extern "C" __declspec(dllimport)
#endif
#include <luacode.h>
#include <lualib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct lua_State lua_State;
typedef struct luaL_Reg luaL_Reg;
// Creates a new userdata tag.
// All this really does is increment a value.
// Use this when you want to utilize the lua_newuserdatatagged functionality
// while keeping the tags synchronized between dlls.
DLUAU_API int dluau_newuserdatatag();
// Creates a new light userdata tag.
// All this really does is increment a value.
// Use this when you want to utilize the lua_pushlightuserdatatagged functionality
// while keeping the tags synchronized between dlls.
DLUAU_API int dluau_newlightuserdatatag();
// When the type is registered this returns the tag.
// Returns 0 when not registered or not a tagged type.
DLUAU_API int dluau_gettagfromtype(const char* tname);
// Registers a type with a tag.
// Returns the tag.
DLUAU_API int dluau_registertypetagged(const char* tname);
// Returns `true` if the type is registered.
DLUAU_API bool dluau_istyperegistered(const char* tname);
// Registers a type.
DLUAU_API void dluau_registertype(const char* tname);
// Searches for the stringatom in the registry
// @param atom the atom to search for
// @return the `string` on found, `nullptr` when not found
DLUAU_API const char* dluau_findstringatom(int atom);
// Registers a stringatom.
// @param L the lua_State
// @param key the key to register
// @return the atom
DLUAU_API int dluau_stringatom(lua_State* L, const char* key);
// Registers a stringatom based on a char pointer and a lenght.
// @param L the lua_State
// @param key the key to register
// @param the key length
// @return the atom
DLUAU_API int dluau_lstringatom(lua_State* L, const char* key, size_t len);
// the dluau require function implementation
DLUAU_API int dluau_require(lua_State* L, const char* name);
DLUAU_API int dluau_lazyrequire(lua_State* L, const char* name);
// Precompiles the source for precompiled features like `nameof()`
// Returns nullptr on failure.
DLUAU_API char* dluau_precompile(const char* src, size_t src_size, size_t* outsize);
// Wrapper function to initialized a lua_State* that is correctly
// set up for dluau.
DLUAU_API lua_State* dluau_newstate();
struct dluau_runoptions {
    // source files to run separated with ','
    const char* scripts;
    // program launch arguments separated with ','
    const char* args;
    // extra global function registry to inject
    // null-terminated
    const luaL_Reg* global_functions;
    // the debug level for the global compile options
    int debug_level;
    // the optimization level for the global compile options
    int optimization_level;
};
// standard run function
DLUAU_API int dluau_run(const dluau_runoptions* opts);
// Registers dluau global functions.
DLUAU_API void dluau_registerglobals(lua_State* L);
// Registers luau and dluau standard libraries.
DLUAU_API void dluau_openlibs(lua_State* L);
DLUAU_API void dluauopen_dlimport(lua_State* L);
DLUAU_API void dluauopen_print(lua_State* L);
DLUAU_API void dluauopen_scan(lua_State* L);
DLUAU_API void dluauopen_task(lua_State* L);
DLUAU_API void dluauopen_os(lua_State* L);
DLUAU_API void dluauopen_cinterop(lua_State* L);
enum dluau_ctaskstatus {
    DLUAU_CTASK_DONE,
    DLUAU_CTASK_CONTINUE,
    DLUAU_CTASK_ERROR,
};
typedef dluau_ctaskstatus(*dluau_ctask)(const char** errmsg);
// Adds a c task to the dluau task scheduler.
DLUAU_API void dluau_addctask(dluau_ctask step_callback);

DLUAU_API void dluauC_pushint(lua_State* L, int v);
DLUAU_API void dluauC_pushuint(lua_State* L, unsigned int v);
DLUAU_API void dluauC_pushshort(lua_State* L, short v);
DLUAU_API void dluauC_pushushort(lua_State* L, unsigned short v);
DLUAU_API void dluauC_pushlong(lua_State* L, long v);
DLUAU_API void dluauC_pushulong(lua_State* L, unsigned long v);
DLUAU_API void dluauC_pushchar(lua_State* L, char v);
DLUAU_API void dluauC_pushuchar(lua_State* L, unsigned char v);
DLUAU_API void dluauC_pushfloat(lua_State* L, float v);

DLUAU_API int dluauC_toint(lua_State* L, int idx);
DLUAU_API unsigned int dluauC_touint(lua_State* L, int idx);
DLUAU_API short dluauC_toshort(lua_State* L, int idx);
DLUAU_API unsigned short dluauC_toushort(lua_State* L, int idx);
DLUAU_API long dluauC_tolong(lua_State* L, int idx);
DLUAU_API unsigned long dluauC_toulong(lua_State* L, int idx);
DLUAU_API char dluauC_tochar(lua_State* L, int idx);
DLUAU_API unsigned char dluauC_touchar(lua_State* L, int idx);
DLUAU_API float dluauC_tofloat(lua_State* L, int idx);

DLUAU_API int dluauC_checkint(lua_State* L, int idx);
DLUAU_API unsigned int dluauC_checkuint(lua_State* L, int idx);
DLUAU_API short dluauC_checkshort(lua_State* L, int idx);
DLUAU_API unsigned short dluauC_checkushort(lua_State* L, int idx);
DLUAU_API long dluauC_checklong(lua_State* L, int idx);
DLUAU_API unsigned long dluauC_checkulong(lua_State* L, int idx);
DLUAU_API char dluauC_checkchar(lua_State* L, int idx);
DLUAU_API unsigned char dluauC_checkuchar(lua_State* L, int idx);
DLUAU_API float dluauC_checkfloat(lua_State* L, int idx);
