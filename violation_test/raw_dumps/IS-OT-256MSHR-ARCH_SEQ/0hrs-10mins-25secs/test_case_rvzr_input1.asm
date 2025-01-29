.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND AL, 77 # instrumentation
AND RCX, 0b111111111111 # instrumentation
LOCK NOT byte ptr [R14 + RCX] 
JNLE .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND DL, -109 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVS RCX, qword ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
CMOVBE ESI, dword ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RAX], 0b1000000000000000000000000000000 # instrumentation
BSF RCX, qword ptr [R14 + RAX] 
JMP .bb_main.2 
.bb_main.2:
AND AL, -36 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNLE DI, word ptr [R14 + RDI] 
AND RBX, 0b111111111111 # instrumentation
OR RSI, qword ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDX], ESI 
AND RDI, 0b111111111111 # instrumentation
CMOVBE RDI, qword ptr [R14 + RDI] 
JMP .bb_main.3 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
TEST word ptr [R14 + RBX], DI 
AND RBX, 0b111111111111 # instrumentation
CMOVP AX, word ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
OR BX, word ptr [R14 + RAX] 
JMP .bb_main.4 
.bb_main.4:
AND BL, 61 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNL CX, word ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
XOR word ptr [R14 + RSI], SI 
AND RSI, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
CMOVBE ESI, dword ptr [R14 + RSI] 
AND RDI, 0b111111111111 # instrumentation
XOR RCX, qword ptr [R14 + RDI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
