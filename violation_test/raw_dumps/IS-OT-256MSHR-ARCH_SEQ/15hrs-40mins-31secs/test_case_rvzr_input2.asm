.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND AL, -35 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVNL RSI, qword ptr [R14 + RCX] 
JNL .bb_main.1 
JMP .bb_main.exit 
.bb_main.1:
AND DL, -29 # instrumentation
AND RSI, 0b111111111111 # instrumentation
NOT qword ptr [R14 + RSI] 
AND RBX, 0b111111111111 # instrumentation
CMOVLE DX, word ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
CMOVNL BX, word ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
CMOVNS RDI, qword ptr [R14 + RSI] 
AND RCX, 0b111111111111 # instrumentation
CMOVLE RSI, qword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
CMOVZ BX, word ptr [R14 + RSI] 
AND RBX, 0b111111111111 # instrumentation
CMOVNS EDX, dword ptr [R14 + RBX] 
JMP .bb_main.2 
.bb_main.2:
AND BL, -21 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVL RCX, qword ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
CMOVNZ CX, word ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RAX], RBX 
JRCXZ .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND CL, -94 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNS EDX, dword ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
CMOVO RSI, qword ptr [R14 + RCX] 
JMP .bb_main.4 
.bb_main.4:
AND RCX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RCX], DI 
AND RBX, 0b111111111111 # instrumentation
CMOVNLE AX, word ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RAX], 46 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
