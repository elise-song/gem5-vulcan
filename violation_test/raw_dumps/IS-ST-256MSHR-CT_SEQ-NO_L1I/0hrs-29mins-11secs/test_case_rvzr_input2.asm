.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDI, 0b111111111111 # instrumentation
LOCK NOT word ptr [R14 + RDI] 
AND RAX, 0b111111111111 # instrumentation
OR word ptr [R14 + RAX], -90 
JMP .bb_main.1 
.bb_main.1:
AND RCX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RCX], RDI 
AND RDX, 0b111111111111 # instrumentation
OR RBX, qword ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RBX], -87 
AND RDI, 0b111111111111 # instrumentation
OR BX, word ptr [R14 + RDI] 
JRCXZ .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND RAX, 0b111111111111 # instrumentation
XOR SI, word ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RBX], 76 
JMP .bb_main.3 
.bb_main.3:
AND RCX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RCX], RCX 
AND RCX, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RCX], 48 
AND RDX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RDX], -85 
AND RCX, 0b111111111111 # instrumentation
CMOVNB SI, word ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RSI], -84 
JZ .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RSI], DL 
AND RDI, 0b111111111111 # instrumentation
XOR word ptr [R14 + RDI], -44 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RCX], RDX 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
