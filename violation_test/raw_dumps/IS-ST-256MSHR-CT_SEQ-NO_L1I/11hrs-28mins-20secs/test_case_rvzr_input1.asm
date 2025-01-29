.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND CL, -84 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVZ ESI, dword ptr [R14 + RDI] 
AND RDX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RDX], DIL 
AND RCX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RCX], -93 
JMP .bb_main.1 
.bb_main.1:
AND RBX, 0b111111111111 # instrumentation
AND CL, byte ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
CMOVBE ECX, dword ptr [R14 + RBX] 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDI], AL 
JMP .bb_main.2 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RBX], ESI 
AND RDI, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RDI], CL 
AND RCX, 0b111111111111 # instrumentation
CMOVL RDX, qword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RSI] 
AND RCX, 0b111111111111 # instrumentation
OR EDI, dword ptr [R14 + RCX] 
LOOPNE .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
TEST byte ptr [R14 + RBX], CL 
AND RAX, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RAX], 1363616947 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDI], SIL 
LOOPE .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RAX, 0b111111111111 # instrumentation
XOR CL, byte ptr [R14 + RAX] 
AND RAX, 0b111111111111 # instrumentation
CMOVLE EDX, dword ptr [R14 + RAX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
