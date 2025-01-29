.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND AL, -95 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVZ RBX, qword ptr [R14 + RBX] 
JNZ .bb_main.1 
JMP .bb_main.4 
.bb_main.1:
AND DL, -24 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVO EBX, dword ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
CMOVNZ CX, word ptr [R14 + RBX] 
JMP .bb_main.2 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
TEST word ptr [R14 + RDI], -19887 
AND RSI, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RSI], RBX 
AND RCX, 0b111111111111 # instrumentation
CMOVNL EBX, dword ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RCX], 21 
JNB .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RSI, 0b111111111111 # instrumentation
AND byte ptr [R14 + RSI], CL 
AND RCX, 0b111111111111 # instrumentation
CMOVNBE DI, word ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RSI], -100 
AND RSI, 0b111111111111 # instrumentation
CMOVBE AX, word ptr [R14 + RSI] 
AND RDX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RDX], -121 
AND RCX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RCX], CX 
AND RDX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDX], 0b1000000000000000000000000000000 # instrumentation
BSR EAX, dword ptr [R14 + RDX] 
JMP .bb_main.4 
.bb_main.4:
AND RAX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RAX], -122 
AND RDX, 0b111111111111 # instrumentation
XOR RDX, qword ptr [R14 + RDX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
