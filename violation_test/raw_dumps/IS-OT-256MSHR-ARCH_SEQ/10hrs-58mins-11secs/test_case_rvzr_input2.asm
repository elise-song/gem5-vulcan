.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RCX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RCX], CL 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RAX], CL 
JLE .bb_main.1 
JMP .bb_main.exit 
.bb_main.1:
AND RDX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDX], CL 
AND RSI, 0b111111111111 # instrumentation
OR word ptr [R14 + RSI], 0b1000000000000000 # instrumentation
BSF BX, word ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RSI], DL 
AND RCX, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RCX], 65 
AND RCX, 0b111111111111 # instrumentation
CMOVNP CX, word ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RSI], BL 
AND RAX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RAX], -124 
AND RAX, 0b111111111111 # instrumentation
OR word ptr [R14 + RAX], 0b1000000000000000 # instrumentation
BSF DI, word ptr [R14 + RAX] 
AND CL, -42 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNL AX, word ptr [R14 + RDI] 
JMP .bb_main.2 
.bb_main.2:
AND RCX, 0b111111111111 # instrumentation
OR word ptr [R14 + RCX], CX 
AND RCX, 0b111111111111 # instrumentation
AND DX, word ptr [R14 + RCX] 
JMP .bb_main.3 
.bb_main.3:
AND RAX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RAX], EDI 
AND RSI, 0b111111111111 # instrumentation
CMOVNP RDX, qword ptr [R14 + RSI] 
JMP .bb_main.4 
.bb_main.4:
AND RDX, 0b111111111111 # instrumentation
OR word ptr [R14 + RDX], 0b1000000000000000 # instrumentation
BSF SI, word ptr [R14 + RDX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
