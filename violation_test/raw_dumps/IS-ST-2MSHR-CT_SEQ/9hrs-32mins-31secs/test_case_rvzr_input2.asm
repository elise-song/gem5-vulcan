.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND BL, -110 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVNB CX, word ptr [R14 + RBX] 
AND RCX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RCX], RBX 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RSI], 71 
JS .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND RCX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RCX], RDX 
AND RDI, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RDI], CL 
AND RDI, 0b111111111111 # instrumentation
OR CX, word ptr [R14 + RDI] 
LOOPE .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND CL, -46 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVO SI, word ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RBX], RBX 
AND RDI, 0b111111111111 # instrumentation
OR word ptr [R14 + RDI], SI 
AND RAX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RAX], EDI 
JMP .bb_main.3 
.bb_main.3:
AND RSI, 0b111111111111 # instrumentation
OR EAX, dword ptr [R14 + RSI] 
AND RDI, 0b111111111111 # instrumentation
CMOVO CX, word ptr [R14 + RDI] 
AND RDX, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RDX], -118 
AND RCX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RCX], -115 
JMP .bb_main.4 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
LOCK NOT byte ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
NOT qword ptr [R14 + RAX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
