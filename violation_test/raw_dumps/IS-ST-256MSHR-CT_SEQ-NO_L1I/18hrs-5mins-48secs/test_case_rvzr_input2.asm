.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RBX], 47 
AND RSI, 0b111111111111 # instrumentation
CMOVZ ESI, dword ptr [R14 + RSI] 
LOOPNE .bb_main.1 
JMP .bb_main.exit 
.bb_main.1:
AND RDI, 0b111111111111 # instrumentation
XOR AL, byte ptr [R14 + RDI] 
AND RBX, 0b111111111111 # instrumentation
CMOVNLE ESI, dword ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RSI], RDI 
JNZ .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RBX], 95 
AND RCX, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RCX], ECX 
AND RDI, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RDI], 71 
AND RSI, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RSI], DIL 
AND RCX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RCX], RAX 
AND RSI, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RSI] 
LOOP .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RDX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDX], RAX 
AND RCX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RCX], DI 
AND RDX, 0b111111111111 # instrumentation
XOR SIL, byte ptr [R14 + RDX] 
JNS .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND DL, 32 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVS DX, word ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RBX], CX 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
