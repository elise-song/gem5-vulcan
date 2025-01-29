.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RBX], BL 
AND RDI, 0b111111111111 # instrumentation
LOCK NOT dword ptr [R14 + RDI] 
JMP .bb_main.1 
.bb_main.1:
AND RCX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RCX], -89 
AND RSI, 0b111111111111 # instrumentation
AND dword ptr [R14 + RSI], -75 
AND RDI, 0b111111111111 # instrumentation
CMOVBE BX, word ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RSI], 9 
JNZ .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RDI], CL 
AND RDX, 0b111111111111 # instrumentation
CMOVNO RBX, qword ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RSI], RSI 
AND RBX, 0b111111111111 # instrumentation
CMOVNS RDX, qword ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
OR word ptr [R14 + RSI], 0b1000000000000000 # instrumentation
BSR DI, word ptr [R14 + RSI] 
JRCXZ .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND DL, -107 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVZ RCX, qword ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RAX], 92 
JMP .bb_main.4 
.bb_main.4:
AND RAX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RAX], BL 
AND RCX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RCX], 78 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RAX], 19 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
