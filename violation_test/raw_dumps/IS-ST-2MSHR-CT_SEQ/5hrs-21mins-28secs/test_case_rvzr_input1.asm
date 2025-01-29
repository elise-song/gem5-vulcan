.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RSI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RSI], EAX 
JMP .bb_main.1 
.bb_main.1:
AND RCX, 0b111111111111 # instrumentation
NOT dword ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RBX], 24 
JNO .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RBX], SI 
AND RDI, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RDI], -86 
AND RSI, 0b111111111111 # instrumentation
CMOVL DI, word ptr [R14 + RSI] 
AND RDI, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RDI], EDX 
AND RBX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RBX], RDX 
AND RDI, 0b111111111111 # instrumentation
CMOVNL RBX, qword ptr [R14 + RDI] 
JMP .bb_main.3 
.bb_main.3:
AND RAX, 0b111111111111 # instrumentation
XOR RDI, qword ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RDX], -73 
AND RSI, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RSI], DL 
AND RCX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RCX], EBX 
JMP .bb_main.4 
.bb_main.4:
AND CL, -98 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNBE EBX, dword ptr [R14 + RDX] 
AND RDI, 0b111111111111 # instrumentation
CMOVB DI, word ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
CMOVNS SI, word ptr [R14 + RSI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
