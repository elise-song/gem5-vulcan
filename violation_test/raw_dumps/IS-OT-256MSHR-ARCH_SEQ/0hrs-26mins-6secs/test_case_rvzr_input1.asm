.intel_syntax noprefix
LEA R14, [R14 + 0] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RAX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RDX], -13 
AND RCX, 0b111111111111 # instrumentation
LOCK NOT dword ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
AND dword ptr [R14 + RDI], 26 
AND RDI, 0b111111111111 # instrumentation
LOCK NOT dword ptr [R14 + RDI] 
JMP .bb_main.1 
.bb_main.1:
JMP .bb_main.2 
.bb_main.2:
AND RDX, 0b111111111111 # instrumentation
OR BL, byte ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
CMOVS RAX, qword ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RCX], CL 
AND RCX, 0b111111111111 # instrumentation
CMOVNO RCX, qword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RSI], RSI 
AND RBX, 0b111111111111 # instrumentation
CMOVS EAX, dword ptr [R14 + RBX] 
JNS .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND DIL, 51 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNO BX, word ptr [R14 + RAX] 
AND RCX, 0b111111111111 # instrumentation
OR word ptr [R14 + RCX], 0b1000000000000000 # instrumentation
BSF AX, word ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RBX], RAX 
JMP .bb_main.4 
.bb_main.4:
AND SIL, -28 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVNZ RCX, qword ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RAX], 1569586836 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 0] # instrumentation
