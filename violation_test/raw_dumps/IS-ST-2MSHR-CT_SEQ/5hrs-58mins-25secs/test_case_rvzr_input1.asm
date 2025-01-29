.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND DL, 89 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNLE RSI, qword ptr [R14 + RAX] 
AND RAX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RAX], BL 
AND RDI, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RDI], -116 
AND RBX, 0b111111111111 # instrumentation
CMOVS DX, word ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDX], BL 
JL .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND CL, -63 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVNS EAX, dword ptr [R14 + RSI] 
AND RCX, 0b111111111111 # instrumentation
OR RBX, qword ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
CMOVB BX, word ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
OR RAX, qword ptr [R14 + RDX] 
JMP .bb_main.2 
.bb_main.2:
AND DL, 113 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNP RCX, qword ptr [R14 + RDI] 
JMP .bb_main.3 
.bb_main.3:
AND RDI, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RDI], RSI 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RBX], 21 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RAX], -64 
AND RDI, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RDI], EAX 
JMP .bb_main.4 
.bb_main.4:
AND AL, -77 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVL RBX, qword ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RAX], RDI 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
