.intel_syntax noprefix
LEA R14, [R14 + 36] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RSI, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RSI], -50 
AND RSI, 0b111111111111 # instrumentation
CMOVNP RCX, qword ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
OR byte ptr [R14 + RSI], AL 
JMP .bb_main.1 
.bb_main.1:
AND AL, -6 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVB RCX, qword ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
CMOVBE DI, word ptr [R14 + RBX] 
JMP .bb_main.2 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RBX], RBX 
AND RDX, 0b111111111111 # instrumentation
CMOVO RSI, qword ptr [R14 + RDX] 
JMP .bb_main.3 
.bb_main.3:
AND RSI, 0b111111111111 # instrumentation
AND byte ptr [R14 + RSI], CL 
AND RDI, 0b111111111111 # instrumentation
CMOVNBE DX, word ptr [R14 + RDI] 
AND RDX, 0b111111111111 # instrumentation
CMOVNP EAX, dword ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
CMOVNP RAX, qword ptr [R14 + RAX] 
AND RCX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RCX], 0b1000000000000000000000000000000 # instrumentation
BSR EDI, dword ptr [R14 + RCX] 
AND AL, -56 # instrumentation
JNL .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RDX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDX], -13 
AND RCX, 0b111111111111 # instrumentation
CMOVNBE EBX, dword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RSI], 84 
AND RSI, 0b111111111111 # instrumentation
LOCK NOT word ptr [R14 + RSI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 36] # instrumentation
