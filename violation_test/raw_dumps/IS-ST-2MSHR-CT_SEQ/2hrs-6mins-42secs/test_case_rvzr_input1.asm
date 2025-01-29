.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RDX], -99 
AND RBX, 0b111111111111 # instrumentation
CMOVNLE RDX, qword ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
CMOVP DI, word ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RDX], DIL 
JMP .bb_main.1 
.bb_main.1:
AND BL, -7 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNBE EBX, dword ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RSI], 69 
AND RSI, 0b111111111111 # instrumentation
AND word ptr [R14 + RSI], -3 
JL .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RCX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RCX], BL 
AND RBX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RBX] 
JMP .bb_main.3 
.bb_main.3:
AND CL, 47 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNZ ECX, dword ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RAX], 3 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RDI], -124 
AND RDI, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDI], -116 
AND RDX, 0b111111111111 # instrumentation
CMOVNL AX, word ptr [R14 + RDX] 
LOOP .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RAX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RAX], AL 
AND RBX, 0b111111111111 # instrumentation
CMOVNL RSI, qword ptr [R14 + RBX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
