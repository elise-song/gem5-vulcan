.intel_syntax noprefix
LEA R14, [R14 + 20] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND CL, -34 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVP ECX, dword ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
AND byte ptr [R14 + RSI], 31 
AND RBX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RBX], -21 
AND RDX, 0b111111111111 # instrumentation
AND DI, word ptr [R14 + RDX] 
JS .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RBX, 0b111111111111 # instrumentation
OR DI, word ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
CMOVNBE BX, word ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
OR qword ptr [R14 + RSI], RCX 
AND RSI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RSI], RDX 
AND RDI, 0b111111111111 # instrumentation
CMOVNLE EAX, dword ptr [R14 + RDI] 
JL .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
OR BX, word ptr [R14 + RDI] 
AND RBX, 0b111111111111 # instrumentation
OR word ptr [R14 + RBX], DI 
JMP .bb_main.3 
.bb_main.3:
AND CL, 17 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVNLE RSI, qword ptr [R14 + RSI] 
AND RCX, 0b111111111111 # instrumentation
AND RBX, qword ptr [R14 + RCX] 
JMP .bb_main.4 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
OR RCX, qword ptr [R14 + RSI] 
AND RBX, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RBX], 275523336 
AND RCX, 0b111111111111 # instrumentation
CMOVNBE ECX, dword ptr [R14 + RCX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 20] # instrumentation
