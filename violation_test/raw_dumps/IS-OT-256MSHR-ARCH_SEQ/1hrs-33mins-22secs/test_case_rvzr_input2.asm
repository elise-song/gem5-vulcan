.intel_syntax noprefix
LEA R14, [R14 + 12] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RBX], CL 
AND RDX, 0b111111111111 # instrumentation
NOT dword ptr [R14 + RDX] 
AND RDI, 0b111111111111 # instrumentation
CMOVNLE SI, word ptr [R14 + RDI] 
JLE .bb_main.1 
JMP .bb_main.exit 
.bb_main.1:
AND BL, -5 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVB RCX, qword ptr [R14 + RDI] 
AND RDX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RDX], RBX 
AND RDX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RDX], 39 
AND RAX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RAX], AL 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RCX], -102 
JLE .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RCX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RCX], 62 
JZ .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RDI, 0b111111111111 # instrumentation
AND RDX, qword ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
CMOVNLE EDI, dword ptr [R14 + RSI] 
AND RDI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RDI], RDI 
AND RCX, 0b111111111111 # instrumentation
CMOVZ RCX, qword ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
CMOVS ESI, dword ptr [R14 + RDI] 
JMP .bb_main.4 
.bb_main.4:
AND BL, 34 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVLE EDX, dword ptr [R14 + RAX] 
AND RDI, 0b111111111111 # instrumentation
OR word ptr [R14 + RDI], 14 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 12] # instrumentation
