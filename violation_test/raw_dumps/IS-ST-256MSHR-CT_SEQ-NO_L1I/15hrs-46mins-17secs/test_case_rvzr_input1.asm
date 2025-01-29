.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
OR word ptr [R14 + RDX], SI 
AND RAX, 0b111111111111 # instrumentation
AND word ptr [R14 + RAX], -31 
AND RCX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RCX], CX 
AND RSI, 0b111111111111 # instrumentation
CMOVZ RBX, qword ptr [R14 + RSI] 
JMP .bb_main.1 
.bb_main.1:
AND RDI, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RDI], 62 
AND RAX, 0b111111111111 # instrumentation
XOR RDI, qword ptr [R14 + RAX] 
JMP .bb_main.2 
.bb_main.2:
AND RAX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RAX], DX 
AND RCX, 0b111111111111 # instrumentation
CMOVNO RCX, qword ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RBX], CL 
AND RSI, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RSI], EAX 
AND RAX, 0b111111111111 # instrumentation
CMOVNS EAX, dword ptr [R14 + RAX] 
JNS .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND DL, -36 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVO RBX, qword ptr [R14 + RDI] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RAX], 86 
AND RAX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RAX], 57 
JNS .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND DL, -106 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVNBE SI, word ptr [R14 + RSI] 
AND RCX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RCX], 101 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
