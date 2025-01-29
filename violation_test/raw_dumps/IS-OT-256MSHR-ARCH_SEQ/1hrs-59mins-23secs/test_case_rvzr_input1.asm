.intel_syntax noprefix
LEA R14, [R14 + 56] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND CL, 97 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNLE RDI, qword ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
CMOVNZ CX, word ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
XOR BL, byte ptr [R14 + RDI] 
LOOPNE .bb_main.1 
JMP .bb_main.4 
.bb_main.1:
AND BL, -95 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVBE AX, word ptr [R14 + RBX] 
AND RDI, 0b111111111111 # instrumentation
CMOVP SI, word ptr [R14 + RDI] 
AND RDX, 0b111111111111 # instrumentation
CMOVL RAX, qword ptr [R14 + RDX] 
AND RDX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RDX], RBX 
AND RSI, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RSI], EAX 
AND RDI, 0b111111111111 # instrumentation
CMOVNO DI, word ptr [R14 + RDI] 
JMP .bb_main.2 
.bb_main.2:
AND RDX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RDX], RDX 
JMP .bb_main.3 
.bb_main.3:
AND RDX, 0b111111111111 # instrumentation
AND CL, byte ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
CMOVS CX, word ptr [R14 + RCX] 
JMP .bb_main.4 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
XOR EDX, dword ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
CMOVO RDI, qword ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDX], 63 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RAX], CL 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 56] # instrumentation
