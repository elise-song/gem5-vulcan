.intel_syntax noprefix
LEA R14, [R14 + 0] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDI, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RDI], 125 
AND RBX, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RBX], -93 
AND RDI, 0b111111111111 # instrumentation
NOT byte ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
CMOVNZ SI, word ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RSI], AL 
JNP .bb_main.1 
JMP .bb_main.4 
.bb_main.1:
AND BL, -48 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVL DX, word ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
CMOVNLE DI, word ptr [R14 + RDI] 
JMP .bb_main.2 
.bb_main.2:
AND RDX, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RDX], EBX 
AND RSI, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RSI], 70 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RSI], RCX 
JZ .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RDX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDX], 0b1000000000000000000000000000000 # instrumentation
BSR RSI, qword ptr [R14 + RDX] 
AND RDX, 0b111111111111 # instrumentation
AND dword ptr [R14 + RDX], 69 
JMP .bb_main.4 
.bb_main.4:
AND RDI, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RDI], 853160470 
AND RDI, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RDI], 112 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RCX], -89 
AND RAX, 0b111111111111 # instrumentation
CMOVNBE EBX, dword ptr [R14 + RAX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 0] # instrumentation
