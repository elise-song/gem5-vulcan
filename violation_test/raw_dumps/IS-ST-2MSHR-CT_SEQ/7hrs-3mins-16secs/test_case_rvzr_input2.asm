.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RDX], 58 
AND RBX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RBX], -57 
AND RDI, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RDI], 105 
JMP .bb_main.1 
.bb_main.1:
AND RDX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RDX], 98 
AND RDI, 0b111111111111 # instrumentation
XOR CX, word ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RDI], -1379321461 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RCX], -49 
JMP .bb_main.2 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
AND ECX, dword ptr [R14 + RBX] 
AND RCX, 0b111111111111 # instrumentation
CMOVBE RDI, qword ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDX], 85 
AND RSI, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RSI], RAX 
JNLE .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND DL, 46 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNL BX, word ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RDX], -38 
JMP .bb_main.4 
.bb_main.4:
AND RCX, 0b111111111111 # instrumentation
XOR DI, word ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
CMOVL CX, word ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RAX], RSI 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
