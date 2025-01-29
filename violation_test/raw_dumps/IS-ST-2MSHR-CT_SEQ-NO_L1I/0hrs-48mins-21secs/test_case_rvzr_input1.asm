.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND CL, 11 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVZ AX, word ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RAX], RDI 
AND RDI, 0b111111111111 # instrumentation
AND byte ptr [R14 + RDI], -62 
AND RDI, 0b111111111111 # instrumentation
NOT word ptr [R14 + RDI] 
JL .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RDX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RDX], RSI 
JNBE .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RBX], -106 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RSI], CL 
AND RAX, 0b111111111111 # instrumentation
CMOVNZ CX, word ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
CMOVNP EDX, dword ptr [R14 + RDX] 
JZ .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND DL, 69 # instrumentation
AND RDX, 0b111111111111 # instrumentation
NOT byte ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
CMOVNBE RDX, qword ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RAX], -92 
AND RDI, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RDI], EAX 
JNZ .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RDI, 0b111111111111 # instrumentation
XOR RBX, qword ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RDI], DL 
AND RCX, 0b111111111111 # instrumentation
CMOVBE AX, word ptr [R14 + RCX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
