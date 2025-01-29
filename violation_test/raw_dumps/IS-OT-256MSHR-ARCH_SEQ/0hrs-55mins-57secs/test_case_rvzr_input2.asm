.intel_syntax noprefix
LEA R14, [R14 + 16] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND DL, -90 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVL RBX, qword ptr [R14 + RDI] 
AND RBX, 0b111111111111 # instrumentation
XOR AX, word ptr [R14 + RBX] 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RCX], DL 
JNP .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RAX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RAX], 28 
AND RCX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RCX], -121 
AND RBX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RBX], 101 
JMP .bb_main.2 
.bb_main.2:
AND RCX, 0b111111111111 # instrumentation
AND DX, word ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
CMOVNZ RAX, qword ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
CMOVO EBX, dword ptr [R14 + RCX] 
JNP .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
OR RSI, qword ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDX], 0b1000000000000000000000000000000 # instrumentation
BSF RCX, qword ptr [R14 + RDX] 
AND CL, 6 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVBE EAX, dword ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
CMOVNO EAX, dword ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
CMOVBE EDX, dword ptr [R14 + RAX] 
JP .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND AL, -6 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVP RAX, qword ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RSI], RAX 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 16] # instrumentation
