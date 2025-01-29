.intel_syntax noprefix
LEA R14, [R14 + 44] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND BL, 115 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVBE ECX, dword ptr [R14 + RAX] 
AND RCX, 0b111111111111 # instrumentation
CMOVNP EBX, dword ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
AND dword ptr [R14 + RBX], EDX 
AND RBX, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RBX], RAX 
AND RBX, 0b111111111111 # instrumentation
AND DX, word ptr [R14 + RBX] 
LOOPNE .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND CL, 20 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNLE DI, word ptr [R14 + RAX] 
AND RAX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RAX], RSI 
AND RBX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RBX], 75 
JLE .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
XOR DI, word ptr [R14 + RDI] 
AND RBX, 0b111111111111 # instrumentation
CMOVNP EBX, dword ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
CMOVNZ EAX, dword ptr [R14 + RSI] 
JNP .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RDI, 0b111111111111 # instrumentation
OR byte ptr [R14 + RDI], CL 
AND RDI, 0b111111111111 # instrumentation
CMOVNS SI, word ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RCX], RDI 
JMP .bb_main.4 
.bb_main.4:
AND RDI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RDI], -25 
AND RBX, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RBX], -48 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 44] # instrumentation
