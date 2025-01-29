.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDI, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RDI], 2122965624 
AND RCX, 0b111111111111 # instrumentation
CMOVB CX, word ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
OR word ptr [R14 + RDX], 0b1000000000000000 # instrumentation
BSF CX, word ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RCX], 3 
JMP .bb_main.1 
.bb_main.1:
AND CL, 106 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNLE CX, word ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
CMOVZ EAX, dword ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
CMOVNZ DI, word ptr [R14 + RSI] 
AND RBX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RBX], BL 
JMP .bb_main.2 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
AND byte ptr [R14 + RSI], -126 
LOOP .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RAX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RAX], 0b1000000000000000000000000000000 # instrumentation
BSF EBX, dword ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDX], 85 
AND RBX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RBX], 67 
AND RDX, 0b111111111111 # instrumentation
CMOVS RDI, qword ptr [R14 + RDX] 
JMP .bb_main.4 
.bb_main.4:
AND RDX, 0b111111111111 # instrumentation
XOR BL, byte ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
CMOVP ESI, dword ptr [R14 + RSI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
