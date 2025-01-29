.intel_syntax noprefix
LEA R14, [R14 + 56] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDI, 0b111111111111 # instrumentation
XOR AL, byte ptr [R14 + RDI] 
JNLE .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RAX, 0b111111111111 # instrumentation
XOR BL, byte ptr [R14 + RAX] 
JP .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND DL, -36 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVNB RDX, qword ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDX], 0b1000000000000000000000000000000 # instrumentation
BSR ESI, dword ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
AND word ptr [R14 + RSI], 22 
AND RDI, 0b111111111111 # instrumentation
CMOVNLE RCX, qword ptr [R14 + RDI] 
AND RAX, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RAX], RCX 
JMP .bb_main.3 
.bb_main.3:
AND RSI, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RSI], -53 
AND RDI, 0b111111111111 # instrumentation
CMOVBE RDI, qword ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
XOR DI, word ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
CMOVO RCX, qword ptr [R14 + RCX] 
JMP .bb_main.4 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
AND word ptr [R14 + RSI], -13 
AND RCX, 0b111111111111 # instrumentation
OR word ptr [R14 + RCX], DI 
AND RBX, 0b111111111111 # instrumentation
AND word ptr [R14 + RBX], -100 
AND RDX, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RDX], -40 
AND RSI, 0b111111111111 # instrumentation
CMOVNL AX, word ptr [R14 + RSI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 56] # instrumentation
