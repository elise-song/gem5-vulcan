.intel_syntax noprefix
LEA R14, [R14 + 16] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RSI, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RSI], 63 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDI], DL 
AND RDI, 0b111111111111 # instrumentation
CMOVO RAX, qword ptr [R14 + RDI] 
JMP .bb_main.1 
.bb_main.1:
AND RDI, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RDI], RBX 
AND RAX, 0b111111111111 # instrumentation
CMOVNS BX, word ptr [R14 + RAX] 
JNBE .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RBX], DL 
AND RSI, 0b111111111111 # instrumentation
CMOVNP RDI, qword ptr [R14 + RSI] 
AND RDI, 0b111111111111 # instrumentation
CMOVBE RSI, qword ptr [R14 + RDI] 
AND RDX, 0b111111111111 # instrumentation
CMOVNL DX, word ptr [R14 + RDX] 
JS .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND BL, -83 # instrumentation
AND RAX, 0b111111111111 # instrumentation
NOT byte ptr [R14 + RAX] 
AND RDI, 0b111111111111 # instrumentation
CMOVBE EAX, dword ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
CMOVNL RSI, qword ptr [R14 + RSI] 
LOOP .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND BL, 107 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNB BX, word ptr [R14 + RDX] 
AND RDX, 0b111111111111 # instrumentation
CMOVLE RSI, qword ptr [R14 + RDX] 
AND RDI, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RDI], AL 
AND RBX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RBX], -40 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 16] # instrumentation
