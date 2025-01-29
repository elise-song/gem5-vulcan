.intel_syntax noprefix
LEA R14, [R14 + 0] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDX], DL 
AND RAX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RAX], SIL 
AND RDX, 0b111111111111 # instrumentation
CMOVNL BX, word ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
LOCK NOT word ptr [R14 + RCX] 
JS .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND AL, 99 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNB EDX, dword ptr [R14 + RDI] 
JNBE .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
AND CL, byte ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
CMOVNB RDX, qword ptr [R14 + RDX] 
AND RDI, 0b111111111111 # instrumentation
AND byte ptr [R14 + RDI], -67 
LOOPNE .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RSI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RSI], RDX 
AND RAX, 0b111111111111 # instrumentation
OR EDX, dword ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RBX], 77 
LOOPNE .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RDI, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RDI], 56 
AND RAX, 0b111111111111 # instrumentation
OR word ptr [R14 + RAX], 23 
AND RCX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RCX], 0b1000000000000000000000000000000 # instrumentation
BSF RSI, qword ptr [R14 + RCX] 
AND AL, -43 # instrumentation
AND RBX, 0b111111111111 # instrumentation
NOT word ptr [R14 + RBX] 
AND RDI, 0b111111111111 # instrumentation
CMOVB CX, word ptr [R14 + RDI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 0] # instrumentation
