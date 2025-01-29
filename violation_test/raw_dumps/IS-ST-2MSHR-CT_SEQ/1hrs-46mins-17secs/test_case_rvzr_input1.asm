.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND CL, 89 # instrumentation
JNP .bb_main.1 
JMP .bb_main.exit 
.bb_main.1:
AND DL, -88 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVL RBX, qword ptr [R14 + RSI] 
AND RBX, 0b111111111111 # instrumentation
CMOVBE BX, word ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RDX], 20 
AND RDI, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RDI], -89 
JMP .bb_main.2 
.bb_main.2:
AND RAX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RAX], 127 
AND RBX, 0b111111111111 # instrumentation
CMOVLE CX, word ptr [R14 + RBX] 
AND RDI, 0b111111111111 # instrumentation
LOCK NOT word ptr [R14 + RDI] 
JMP .bb_main.3 
.bb_main.3:
AND RAX, 0b111111111111 # instrumentation
OR word ptr [R14 + RAX], 0b1000000000000000 # instrumentation
BSR AX, word ptr [R14 + RAX] 
AND BL, 106 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVNBE RSI, qword ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
CMOVLE RDI, qword ptr [R14 + RDI] 
AND RBX, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RBX], EAX 
JMP .bb_main.4 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RSI], -18 
AND RDX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RDX], -16 
AND RCX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RCX], SIL 
AND RDX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RDX], BL 
AND RAX, 0b111111111111 # instrumentation
CMOVP ECX, dword ptr [R14 + RAX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
