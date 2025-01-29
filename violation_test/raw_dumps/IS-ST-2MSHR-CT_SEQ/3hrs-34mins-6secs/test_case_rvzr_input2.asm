.intel_syntax noprefix
LEA R14, [R14 + 44] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
OR DL, byte ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RBX], DL 
AND RDX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RDX], EBX 
JMP .bb_main.1 
.bb_main.1:
AND RSI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RSI], 0b1000000000000000000000000000000 # instrumentation
BSR EBX, dword ptr [R14 + RSI] 
AND AL, -77 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVB RCX, qword ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
OR ESI, dword ptr [R14 + RBX] 
JO .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
AND word ptr [R14 + RDI], AX 
AND RBX, 0b111111111111 # instrumentation
CMOVNL RSI, qword ptr [R14 + RBX] 
AND RCX, 0b111111111111 # instrumentation
CMOVO RSI, qword ptr [R14 + RCX] 
JLE .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RDX, 0b111111111111 # instrumentation
LOCK NOT word ptr [R14 + RDX] 
AND RDX, 0b111111111111 # instrumentation
OR ECX, dword ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
XOR EBX, dword ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
AND dword ptr [R14 + RAX], EDX 
JMP .bb_main.4 
.bb_main.4:
AND RDI, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RDI], 32 
AND RBX, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RBX], RAX 
AND RDX, 0b111111111111 # instrumentation
XOR AL, byte ptr [R14 + RDX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 44] # instrumentation
