.intel_syntax noprefix
LEA R14, [R14 + 40] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
TEST word ptr [R14 + RDX], DX 
AND RDX, 0b111111111111 # instrumentation
CMOVNS BX, word ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RBX], EAX 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RAX], CL 
JNLE .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RCX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RCX], 92 
AND RSI, 0b111111111111 # instrumentation
AND RSI, qword ptr [R14 + RSI] 
AND RDI, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RDI], 57 
JNL .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDI], EDI 
AND RSI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RSI], EDX 
AND RDX, 0b111111111111 # instrumentation
CMOVNBE EBX, dword ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RBX], -120 
JS .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RAX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RAX], CL 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RSI], 37 
JMP .bb_main.4 
.bb_main.4:
AND RDX, 0b111111111111 # instrumentation
AND DX, word ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
CMOVL BX, word ptr [R14 + RSI] 
AND RDX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RDX], EAX 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 40] # instrumentation
