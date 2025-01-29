.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND CL, 46 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNB RDI, qword ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RSI], DIL 
AND RDX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDX], 0b1000000000000000000000000000000 # instrumentation
BSF RDI, qword ptr [R14 + RDX] 
JMP .bb_main.1 
.bb_main.1:
AND RSI, 0b111111111111 # instrumentation
OR qword ptr [R14 + RSI], RCX 
AND RSI, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RSI], -111 
AND RDX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RDX], CL 
JMP .bb_main.2 
.bb_main.2:
AND RSI, 0b111111111111 # instrumentation
OR word ptr [R14 + RSI], 0b1000000000000000 # instrumentation
BSR AX, word ptr [R14 + RSI] 
AND RCX, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RCX], RBX 
AND RSI, 0b111111111111 # instrumentation
LOCK NOT word ptr [R14 + RSI] 
LOOPNE .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
NOT byte ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
NOT byte ptr [R14 + RSI] 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RCX], -82 
AND RCX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RCX], EBX 
AND RCX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RCX], AL 
JMP .bb_main.4 
.bb_main.4:
AND BL, -91 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNLE BX, word ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RAX], CL 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
