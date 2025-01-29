.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND AL, -62 # instrumentation
JP .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RDX, 0b111111111111 # instrumentation
AND CL, byte ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RAX], ESI 
AND RBX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RBX], -36 
AND RDX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RDX], RSI 
JMP .bb_main.2 
.bb_main.2:
AND RCX, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RCX], -42 
AND RCX, 0b111111111111 # instrumentation
OR word ptr [R14 + RCX], 80 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RSI], SI 
JNL .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND CL, 86 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVO RCX, qword ptr [R14 + RBX] 
AND RDI, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RDI], 81 
AND RDX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RDX], DL 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RBX], -79 
JMP .bb_main.4 
.bb_main.4:
AND RBX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RBX], DL 
AND RDI, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RDI], DX 
AND RCX, 0b111111111111 # instrumentation
CMOVP SI, word ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
AND AL, byte ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
CMOVNS RSI, qword ptr [R14 + RSI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
