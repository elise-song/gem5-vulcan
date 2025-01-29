.intel_syntax noprefix
LEA R14, [R14 + 52] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND DL, -2 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVNL RBX, qword ptr [R14 + RSI] 
AND RDX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RDX], BL 
LOOPNE .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND CL, -128 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNL RSI, qword ptr [R14 + RAX] 
JL .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND DL, 6 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVZ RCX, qword ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
XOR EAX, dword ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
OR word ptr [R14 + RCX], 0b1000000000000000 # instrumentation
BSR SI, word ptr [R14 + RCX] 
AND AL, -22 # instrumentation
JNL .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RSI, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RSI], AL 
AND RAX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RAX], -28 
AND RSI, 0b111111111111 # instrumentation
OR word ptr [R14 + RSI], -125 
AND RCX, 0b111111111111 # instrumentation
CMOVS RDX, qword ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
CMOVZ DI, word ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
CMOVNB EDI, dword ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
CMOVNLE RBX, qword ptr [R14 + RAX] 
JMP .bb_main.4 
.bb_main.4:
AND BL, -82 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNO RAX, qword ptr [R14 + RAX] 
AND RCX, 0b111111111111 # instrumentation
CMOVO ECX, dword ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
CMOVNP EDX, dword ptr [R14 + RDX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 52] # instrumentation
