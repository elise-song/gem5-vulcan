.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDI], 0b1000000000000000000000000000000 # instrumentation
BSF ECX, dword ptr [R14 + RDI] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RAX], -102 
JNS .bb_main.1 
JMP .bb_main.4 
.bb_main.1:
AND RSI, 0b111111111111 # instrumentation
OR RDI, qword ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RSI], RBX 
AND RCX, 0b111111111111 # instrumentation
CMOVB EDX, dword ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
NOT word ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RCX], -54 
JMP .bb_main.2 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RDI], 19 
AND RSI, 0b111111111111 # instrumentation
CMOVNZ EDX, dword ptr [R14 + RSI] 
AND RDX, 0b111111111111 # instrumentation
CMOVB AX, word ptr [R14 + RDX] 
JNS .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND AL, -97 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVNBE RSI, qword ptr [R14 + RCX] 
JB .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RCX, 0b111111111111 # instrumentation
OR AL, byte ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RBX], AL 
AND RCX, 0b111111111111 # instrumentation
OR CX, word ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
CMOVNL RBX, qword ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
CMOVNZ DI, word ptr [R14 + RDX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
