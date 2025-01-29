.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
AND word ptr [R14 + RBX], -18 
AND RDI, 0b111111111111 # instrumentation
CMOVNS AX, word ptr [R14 + RDI] 
JMP .bb_main.1 
.bb_main.1:
AND BL, 12 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNS RAX, qword ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
CMOVLE EBX, dword ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
OR word ptr [R14 + RSI], BX 
JNP .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RBX], 126 
JMP .bb_main.3 
.bb_main.3:
AND RAX, 0b111111111111 # instrumentation
XOR BL, byte ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RSI], -61 
AND RAX, 0b111111111111 # instrumentation
CMOVNP CX, word ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
AND AL, byte ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
AND word ptr [R14 + RSI], AX 
JMP .bb_main.4 
.bb_main.4:
AND RDI, 0b111111111111 # instrumentation
OR byte ptr [R14 + RDI], 112 
AND RAX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RAX], 17 
AND RBX, 0b111111111111 # instrumentation
CMOVNL RSI, qword ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
CMOVL RCX, qword ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RSI], 98 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
