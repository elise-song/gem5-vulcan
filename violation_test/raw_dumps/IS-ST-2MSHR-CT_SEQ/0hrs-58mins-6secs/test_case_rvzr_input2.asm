.intel_syntax noprefix
LEA R14, [R14 + 56] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RBX], 81 
AND RDX, 0b111111111111 # instrumentation
OR CX, word ptr [R14 + RDX] 
JNBE .bb_main.1 
JMP .bb_main.4 
.bb_main.1:
AND AL, -98 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNZ SI, word ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RSI], -114 
AND RAX, 0b111111111111 # instrumentation
XOR EDX, dword ptr [R14 + RAX] 
JNS .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND BL, 121 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVS ESI, dword ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
XOR ESI, dword ptr [R14 + RBX] 
AND RDI, 0b111111111111 # instrumentation
CMOVNL RSI, qword ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RCX], RDX 
AND RSI, 0b111111111111 # instrumentation
CMOVNB BX, word ptr [R14 + RSI] 
JNBE .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RBX], -114 
AND RAX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RAX], 0b1000000000000000000000000000000 # instrumentation
BSF ESI, dword ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
CMOVNZ RCX, qword ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RSI], EDX 
JMP .bb_main.4 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
AND qword ptr [R14 + RSI], 95 
AND RSI, 0b111111111111 # instrumentation
CMOVBE RDI, qword ptr [R14 + RSI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 56] # instrumentation
