.intel_syntax noprefix
LEA R14, [R14 + 4] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND AL, -112 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVBE CX, word ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
CMOVO DX, word ptr [R14 + RDI] 
AND RDX, 0b111111111111 # instrumentation
CMOVNP EDX, dword ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
OR EDX, dword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
OR byte ptr [R14 + RSI], CL 
JMP .bb_main.1 
.bb_main.1:
AND RBX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RBX], 39 
AND RCX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RCX], 122 
JS .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RSI, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RSI], CX 
AND RCX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RCX], AX 
AND RCX, 0b111111111111 # instrumentation
CMOVNL RAX, qword ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RCX], 65 
AND RCX, 0b111111111111 # instrumentation
CMOVNS DI, word ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
LOCK NOT word ptr [R14 + RDI] 
JNP .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND DL, -117 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNBE RDI, qword ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RAX], -63 
JL .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RDX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RDX], RCX 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 4] # instrumentation
