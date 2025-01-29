.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
OR qword ptr [R14 + RSI], 104 
AND RSI, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RSI], DX 
JMP .bb_main.1 
.bb_main.1:
AND BL, -2 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVB ESI, dword ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
NOT dword ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RBX], 82 
JNO .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
OR word ptr [R14 + RDI], SI 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RAX], 111 
AND RAX, 0b111111111111 # instrumentation
CMOVNO ESI, dword ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
OR ECX, dword ptr [R14 + RBX] 
JRCXZ .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND CL, 115 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNL AX, word ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
CMOVBE EDX, dword ptr [R14 + RAX] 
AND RCX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RCX], 116 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RBX], 100 
AND RDX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDX], 106 
JNS .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND AL, 85 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNL ESI, dword ptr [R14 + RDI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
