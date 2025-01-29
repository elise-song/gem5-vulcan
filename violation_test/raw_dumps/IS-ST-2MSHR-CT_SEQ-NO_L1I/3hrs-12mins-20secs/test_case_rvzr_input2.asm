.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RBX], ECX 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RDI], AX 
AND RSI, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RSI], AL 
LOOPE .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND RSI, 0b111111111111 # instrumentation
NOT byte ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
TEST word ptr [R14 + RSI], 27043 
AND RCX, 0b111111111111 # instrumentation
XOR CL, byte ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RCX], 0b1000000000000000000000000000000 # instrumentation
BSR EBX, dword ptr [R14 + RCX] 
LOOP .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND RAX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RAX], 0b1000000000000000000000000000000 # instrumentation
BSR EDX, dword ptr [R14 + RAX] 
AND RAX, 0b111111111111 # instrumentation
XOR BX, word ptr [R14 + RAX] 
AND RCX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RCX], 100 
JNZ .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RBX], CL 
AND RDI, 0b111111111111 # instrumentation
OR word ptr [R14 + RDI], 0b1000000000000000 # instrumentation
BSF BX, word ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RCX], RDI 
AND RBX, 0b111111111111 # instrumentation
CMOVS RAX, qword ptr [R14 + RBX] 
JMP .bb_main.4 
.bb_main.4:
AND RBX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RBX], EDX 
AND RDX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RDX], -61 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
