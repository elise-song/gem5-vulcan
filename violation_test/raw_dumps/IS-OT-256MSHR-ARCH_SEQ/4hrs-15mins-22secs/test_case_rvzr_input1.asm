.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDI, 0b111111111111 # instrumentation
TEST word ptr [R14 + RDI], DI 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RBX], BX 
AND RAX, 0b111111111111 # instrumentation
CMOVB CX, word ptr [R14 + RAX] 
JMP .bb_main.1 
.bb_main.1:
AND RCX, 0b111111111111 # instrumentation
OR CL, byte ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
CMOVS RDX, qword ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RBX], ESI 
AND RBX, 0b111111111111 # instrumentation
XOR EDX, dword ptr [R14 + RBX] 
JMP .bb_main.2 
.bb_main.2:
AND RDX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RDX], 52 
AND RDX, 0b111111111111 # instrumentation
CMOVZ CX, word ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RBX], 0b1000000000000000000000000000000 # instrumentation
BSR EDI, dword ptr [R14 + RBX] 
JMP .bb_main.3 
.bb_main.3:
AND DL, 7 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNB EBX, dword ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
AND qword ptr [R14 + RSI], RAX 
JMP .bb_main.4 
.bb_main.4:
AND BL, -21 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVNZ BX, word ptr [R14 + RSI] 
AND RBX, 0b111111111111 # instrumentation
AND AX, word ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
TEST word ptr [R14 + RBX], -14766 
AND RAX, 0b111111111111 # instrumentation
NOT qword ptr [R14 + RAX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
