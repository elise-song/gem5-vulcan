.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RBX], -48 
AND RDI, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RDI], RDX 
AND RBX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RBX], -115 
JNZ .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND RCX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RCX], AL 
AND RBX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RBX], -46 
JMP .bb_main.2 
.bb_main.2:
AND RAX, 0b111111111111 # instrumentation
AND DI, word ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
OR word ptr [R14 + RSI], -104 
JMP .bb_main.3 
.bb_main.3:
AND CL, 81 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNZ RCX, qword ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
CMOVNZ DI, word ptr [R14 + RBX] 
AND RCX, 0b111111111111 # instrumentation
CMOVL ESI, dword ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RAX], DIL 
AND RBX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RBX], RAX 
JNZ .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND DL, 36 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVB EBX, dword ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RDX], DL 
AND RBX, 0b111111111111 # instrumentation
CMOVS RBX, qword ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RBX], DIL 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
