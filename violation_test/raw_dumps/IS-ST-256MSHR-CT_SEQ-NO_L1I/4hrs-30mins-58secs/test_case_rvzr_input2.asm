.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND CL, -124 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVS RBX, qword ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
CMOVNP DI, word ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
CMOVNS SI, word ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
CMOVLE SI, word ptr [R14 + RSI] 
JNZ .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RAX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RAX], 67 
AND RAX, 0b111111111111 # instrumentation
CMOVNO AX, word ptr [R14 + RAX] 
AND RAX, 0b111111111111 # instrumentation
CMOVNLE CX, word ptr [R14 + RAX] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RAX], EAX 
AND RAX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RAX], RCX 
JMP .bb_main.2 
.bb_main.2:
AND RCX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RCX], -98 
JMP .bb_main.3 
.bb_main.3:
AND RDX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDX], 75 
AND RSI, 0b111111111111 # instrumentation
XOR RAX, qword ptr [R14 + RSI] 
AND RBX, 0b111111111111 # instrumentation
XOR DL, byte ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RDX], CL 
AND RDI, 0b111111111111 # instrumentation
CMOVNBE DX, word ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
XOR RDI, qword ptr [R14 + RDI] 
JMP .bb_main.4 
.bb_main.4:
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
