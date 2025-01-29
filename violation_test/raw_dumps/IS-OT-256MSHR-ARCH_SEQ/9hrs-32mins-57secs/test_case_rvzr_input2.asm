.intel_syntax noprefix
LEA R14, [R14 + 32] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RCX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RCX], CL 
AND RAX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RAX], -115 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RCX], 88 
JMP .bb_main.1 
.bb_main.1:
AND RDX, 0b111111111111 # instrumentation
LOCK NOT word ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RCX], -46 
AND RCX, 0b111111111111 # instrumentation
TEST word ptr [R14 + RCX], DX 
JMP .bb_main.2 
.bb_main.2:
AND RDX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RDX], AX 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RSI], -81 
JNS .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RDI, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RDI], DX 
AND RCX, 0b111111111111 # instrumentation
CMOVNL BX, word ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
CMOVNZ ESI, dword ptr [R14 + RBX] 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RDI], 9 
JMP .bb_main.4 
.bb_main.4:
AND SIL, -54 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVNLE ESI, dword ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RSI], BL 
AND RAX, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RAX], EBX 
AND RSI, 0b111111111111 # instrumentation
AND word ptr [R14 + RSI], -29 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 32] # instrumentation
