.intel_syntax noprefix
LEA R14, [R14 + 28] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RCX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RCX], 93 
JLE .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RSI, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RSI], BL 
AND RBX, 0b111111111111 # instrumentation
LOCK NOT dword ptr [R14 + RBX] 
JO .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND SIL, -87 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVNBE CX, word ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
OR byte ptr [R14 + RSI], CL 
AND RAX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RAX], SI 
AND RBX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RBX], DL 
JNZ .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RCX, 0b111111111111 # instrumentation
XOR BL, byte ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
LOCK NOT dword ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
TEST word ptr [R14 + RSI], 16366 
AND RAX, 0b111111111111 # instrumentation
OR RBX, qword ptr [R14 + RAX] 
JP .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RDI, 0b111111111111 # instrumentation
OR SIL, byte ptr [R14 + RDI] 
AND RBX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RBX], BL 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RCX], RSI 
AND RAX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RAX], 27 
AND RDX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDX], AL 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 28] # instrumentation
