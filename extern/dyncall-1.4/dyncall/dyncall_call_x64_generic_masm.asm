; auto-generated by gen-masm.sh
.CODE
dcCall_x64_sysv PROC
OPTION PROLOGUE:NONE, EPILOGUE:NONE
 push RBP
 push RBX
 mov RBP,RSP
 mov RBX,R8
 movsd XMM0,qword ptr [RCX+0]
 movsd XMM1,qword ptr [RCX+8]
 movsd XMM2,qword ptr [RCX+16]
 movsd XMM3,qword ptr [RCX+24]
 movsd XMM4,qword ptr [RCX+32]
 movsd XMM5,qword ptr [RCX+40]
 movsd XMM6,qword ptr [RCX+48]
 movsd XMM7,qword ptr [RCX+56]
 add RDI,31
 and RDI,-32
 add RDI,8
 sub RSP,RDI
 mov RCX,RDI
 mov RDI,RSP
 rep movsb
 mov RDI,qword ptr [RDX+0]
 mov RSI,qword ptr [RDX+8]
 mov RCX,qword ptr [RDX+24]
 mov R8,qword ptr [RDX+32]
 mov R9,qword ptr [RDX+40]
 mov RDX,qword ptr [RDX+16]
 mov AL,8
 call RBX
 mov RSP,RBP
 pop RBX
 pop RBP
 ret
dcCall_x64_sysv ENDP
dcCall_x64_sysv_aggr PROC
OPTION PROLOGUE:NONE, EPILOGUE:NONE
 push R9
 call dcCall_x64_sysv
 pop R9
 mov qword ptr [R9+0],RAX
 mov qword ptr [R9+8],RDX
 movsd qword ptr [R9+16],XMM0
 movsd qword ptr [R9+24],XMM1
 ret
dcCall_x64_sysv_aggr ENDP
dcCall_x64_win64 PROC FRAME
OPTION EPILOGUE:NONE
 push RBP
 .pushreg RBP
 push RSI
 .pushreg RSI
 push RDI
 .pushreg RDI
 mov RBP,RSP
 .setframe RBP, 0
 .endprolog
 add RCX,15
 and RCX,-16
 sub RSP,RCX
 mov RSI,RDX
 mov RDI,RSP
 mov RAX,R9
 rep movsb
 mov RCX,qword ptr [R8+0]
 mov RDX,qword ptr [R8+8]
 mov R9,qword ptr [R8+24]
 mov R8,qword ptr [R8+16]
 movd XMM0,RCX
 movd XMM1,RDX
 movd XMM2,R8
 movd XMM3,R9
 push R9
 push R8
 push RDX
 push RCX
 call RAX
 mov RSP,RBP
 pop RDI
 pop RSI
 pop RBP
 ret
dcCall_x64_win64 ENDP
dcCall_x64_win64_aggr PROC
OPTION PROLOGUE:NONE, EPILOGUE:NONE
 sub RSP,8
 call dcCall_x64_win64
 add RSP,8
 mov R8,qword ptr [RSP+40]
 mov qword ptr [R8+0],RAX
 ret
dcCall_x64_win64_aggr ENDP
dcCall_x64_syscall_sysv PROC
OPTION PROLOGUE:NONE, EPILOGUE:NONE
 mov RAX,RSI
 mov R9,qword ptr [RDI+40]
 mov R8,qword ptr [RDI+32]
 mov R10,qword ptr [RDI+24]
 mov RDX,qword ptr [RDI+16]
 mov RSI,qword ptr [RDI+8]
 mov RDI,qword ptr [RDI+0]
 SYSCALL
 ret
dcCall_x64_syscall_sysv ENDP
END
