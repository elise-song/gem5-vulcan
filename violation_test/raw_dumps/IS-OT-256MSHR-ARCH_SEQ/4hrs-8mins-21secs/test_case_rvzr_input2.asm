.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RBX], -109 
AND RBX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RBX], 82 
AND RCX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RCX], BX 
JBE .bb_main.1 
JMP .bb_main.4 
.bb_main.1:
AND DL, -50 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVO RBX, qword ptr [R14 + RSI] 
JRCXZ .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RSI, 0b111111111111 # instrumentation
OR qword ptr [R14 + RSI], RCX 
JNO .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RCX, 0b111111111111 # instrumentation
NOT dword ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RBX], -19 
AND RDX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDX], 123 
AND RDI, 0b111111111111 # instrumentation
CMOVNZ RAX, qword ptr [R14 + RDI] 
JMP .bb_main.4 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RSI], RBX 
AND RCX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RCX], 34 
AND RBX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RBX], -102 
AND RSI, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RSI], DL 
AND RCX, 0b111111111111 # instrumentation
CMOVNL RDX, qword ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
AND BX, word ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
CMOVNS RCX, qword ptr [R14 + RCX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
