.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RAX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RAX], RCX 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RBX], RDI 
AND RDX, 0b111111111111 # instrumentation
AND EBX, dword ptr [R14 + RDX] 
JMP .bb_main.1 
.bb_main.1:
AND RDI, 0b111111111111 # instrumentation
AND dword ptr [R14 + RDI], -83 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RSI], CX 
AND RDX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RDX], -115 
JNP .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RAX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RAX], RCX 
AND RCX, 0b111111111111 # instrumentation
AND word ptr [R14 + RCX], DX 
JMP .bb_main.3 
.bb_main.3:
AND AL, 95 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVLE RSI, qword ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
OR word ptr [R14 + RAX], AX 
AND RCX, 0b111111111111 # instrumentation
CMOVNO EAX, dword ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDI], 0b1000000000000000000000000000000 # instrumentation
BSF RBX, qword ptr [R14 + RDI] 
AND CL, -48 # instrumentation
JO .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND BL, 95 # instrumentation
AND RDX, 0b111111111111 # instrumentation
NOT qword ptr [R14 + RDX] 
AND RDI, 0b111111111111 # instrumentation
CMOVO RCX, qword ptr [R14 + RDI] 
AND RBX, 0b111111111111 # instrumentation
CMOVNS RDI, qword ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
XOR DL, byte ptr [R14 + RAX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
