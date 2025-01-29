.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RBX], 119 
JMP .bb_main.1 
.bb_main.1:
AND RCX, 0b111111111111 # instrumentation
OR CX, word ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
CMOVNL ECX, dword ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDI], 9 
JNO .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND RSI, 0b111111111111 # instrumentation
AND byte ptr [R14 + RSI], DL 
AND RAX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RAX], CL 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RCX], -106 
AND RSI, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RSI], EDX 
AND RBX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RBX], 0b1000000000000000000000000000000 # instrumentation
BSR ECX, dword ptr [R14 + RBX] 
AND AL, 87 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVL RDI, qword ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
CMOVL CX, word ptr [R14 + RSI] 
JNP .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RBX], RBX 
AND RSI, 0b111111111111 # instrumentation
AND byte ptr [R14 + RSI], DIL 
JMP .bb_main.4 
.bb_main.4:
AND AL, 62 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVNP BX, word ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDX], RBX 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
