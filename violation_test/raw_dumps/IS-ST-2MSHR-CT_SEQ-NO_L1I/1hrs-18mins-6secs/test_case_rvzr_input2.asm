.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RAX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RAX], BL 
AND RBX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RBX], CL 
AND RBX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RBX], 33 
JMP .bb_main.1 
.bb_main.1:
AND RAX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RAX], 76 
AND RDI, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RDI], AX 
AND RBX, 0b111111111111 # instrumentation
CMOVZ RSI, qword ptr [R14 + RBX] 
JMP .bb_main.2 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RDI], -79 
AND RSI, 0b111111111111 # instrumentation
CMOVS DX, word ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
CMOVS EDX, dword ptr [R14 + RAX] 
AND RDI, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RDI], -13 
AND RDX, 0b111111111111 # instrumentation
XOR BL, byte ptr [R14 + RDX] 
JMP .bb_main.3 
.bb_main.3:
AND DL, -43 # instrumentation
AND RSI, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RSI] 
JNS .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RBX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RBX], 0b1000000000000000000000000000000 # instrumentation
BSF RCX, qword ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RAX], RBX 
AND RAX, 0b111111111111 # instrumentation
CMOVNS RAX, qword ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RDX], -108 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
