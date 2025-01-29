.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RDX], 93 
AND RBX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RBX], BL 
AND RDI, 0b111111111111 # instrumentation
CMOVNZ BX, word ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
CMOVNBE EDX, dword ptr [R14 + RCX] 
JMP .bb_main.1 
.bb_main.1:
AND CL, -118 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVO EDI, dword ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
CMOVNLE RAX, qword ptr [R14 + RDI] 
AND RBX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RBX], SIL 
JNZ .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
AND AL, byte ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
CMOVNP RDX, qword ptr [R14 + RBX] 
JMP .bb_main.3 
.bb_main.3:
AND RCX, 0b111111111111 # instrumentation
AND EAX, dword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
CMOVNZ DX, word ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
CMOVBE RBX, qword ptr [R14 + RAX] 
AND RCX, 0b111111111111 # instrumentation
CMOVNLE EDX, dword ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RAX], -85 
JNLE .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RBX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RBX], -13 
AND RCX, 0b111111111111 # instrumentation
LOCK NOT dword ptr [R14 + RCX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
