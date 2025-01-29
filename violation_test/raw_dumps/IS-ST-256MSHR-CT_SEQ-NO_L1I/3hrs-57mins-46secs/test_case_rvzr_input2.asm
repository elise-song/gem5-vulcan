.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RBX], -100 
AND RAX, 0b111111111111 # instrumentation
OR AL, byte ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RBX], EDX 
AND RDX, 0b111111111111 # instrumentation
CMOVNLE ECX, dword ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
CMOVP EBX, dword ptr [R14 + RSI] 
JMP .bb_main.1 
.bb_main.1:
AND RAX, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RAX], EAX 
LOOPNE .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND RSI, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RSI], 33 
AND RAX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RAX], 42 
JMP .bb_main.3 
.bb_main.3:
AND DL, 28 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNP RCX, qword ptr [R14 + RAX] 
AND RDI, 0b111111111111 # instrumentation
AND dword ptr [R14 + RDI], EBX 
AND RBX, 0b111111111111 # instrumentation
AND word ptr [R14 + RBX], 81 
AND RSI, 0b111111111111 # instrumentation
CMOVNS RAX, qword ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
NOT qword ptr [R14 + RSI] 
JB .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RCX, 0b111111111111 # instrumentation
XOR RSI, qword ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RCX], EDI 
AND RAX, 0b111111111111 # instrumentation
CMOVB EAX, dword ptr [R14 + RAX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
