.intel_syntax noprefix
LEA R14, [R14 + 24] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND DIL, 93 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVZ CX, word ptr [R14 + RBX] 
AND RCX, 0b111111111111 # instrumentation
LOCK NOT dword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
AND qword ptr [R14 + RSI], RCX 
JNL .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND RSI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RSI], RCX 
AND RBX, 0b111111111111 # instrumentation
CMOVNZ RDX, qword ptr [R14 + RBX] 
AND RCX, 0b111111111111 # instrumentation
XOR ECX, dword ptr [R14 + RCX] 
JZ .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
OR byte ptr [R14 + RDI], DIL 
AND RDX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDX], 0b1000000000000000000000000000000 # instrumentation
BSR EDX, dword ptr [R14 + RDX] 
JMP .bb_main.3 
.bb_main.3:
AND SIL, 84 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNBE ESI, dword ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
OR RDI, qword ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
CMOVS RBX, qword ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
OR CX, word ptr [R14 + RAX] 
JLE .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RSI], 0b1000000000000000000000000000000 # instrumentation
BSR EAX, dword ptr [R14 + RSI] 
AND DIL, -32 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNP RDX, qword ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
CMOVNZ ECX, dword ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
CMOVNP ECX, dword ptr [R14 + RAX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 24] # instrumentation
