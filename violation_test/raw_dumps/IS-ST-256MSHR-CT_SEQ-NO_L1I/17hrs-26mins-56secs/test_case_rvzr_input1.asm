.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDI, 0b111111111111 # instrumentation
XOR RCX, qword ptr [R14 + RDI] 
AND RAX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RAX], CX 
AND RCX, 0b111111111111 # instrumentation
CMOVNO SI, word ptr [R14 + RCX] 
JMP .bb_main.1 
.bb_main.1:
AND RBX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RBX], ECX 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RAX], 99 
AND RAX, 0b111111111111 # instrumentation
OR word ptr [R14 + RAX], CX 
AND RBX, 0b111111111111 # instrumentation
CMOVNLE RAX, qword ptr [R14 + RBX] 
JNS .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND BL, -71 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVZ RDI, qword ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RAX] 
LOOPNE .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RCX, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RCX], EDI 
AND RCX, 0b111111111111 # instrumentation
CMOVNO BX, word ptr [R14 + RCX] 
JNL .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RBX, 0b111111111111 # instrumentation
XOR RDI, qword ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
AND ESI, dword ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RSI], 826544163 
AND RDI, 0b111111111111 # instrumentation
CMOVNBE RSI, qword ptr [R14 + RDI] 
AND RAX, 0b111111111111 # instrumentation
CMOVP AX, word ptr [R14 + RAX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
