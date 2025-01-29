.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RAX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RAX], DX 
AND RSI, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RSI], RSI 
AND RCX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RCX], CL 
AND RAX, 0b111111111111 # instrumentation
CMOVNB RSI, qword ptr [R14 + RAX] 
JNO .bb_main.1 
JMP .bb_main.4 
.bb_main.1:
AND DL, 116 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVNBE AX, word ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
CMOVNL AX, word ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
TEST word ptr [R14 + RCX], -31023 
AND RAX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RAX] 
JMP .bb_main.2 
.bb_main.2:
AND CL, -107 # instrumentation
AND RCX, 0b111111111111 # instrumentation
LOCK NOT byte ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
CMOVNL RDX, qword ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
CMOVNB ESI, dword ptr [R14 + RAX] 
AND RDI, 0b111111111111 # instrumentation
OR word ptr [R14 + RDI], -45 
JMP .bb_main.3 
.bb_main.3:
JRCXZ .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND BL, -25 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVZ RBX, qword ptr [R14 + RAX] 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RDI], 86 
AND RDI, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RDI], SI 
AND RSI, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RSI], 14 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
