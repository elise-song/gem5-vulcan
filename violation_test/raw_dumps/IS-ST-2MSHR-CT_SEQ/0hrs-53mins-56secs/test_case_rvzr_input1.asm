.intel_syntax noprefix
LEA R14, [R14 + 20] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDI, 0b111111111111 # instrumentation
OR byte ptr [R14 + RDI], AL 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDI], AL 
AND RDI, 0b111111111111 # instrumentation
NOT dword ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
CMOVP ESI, dword ptr [R14 + RSI] 
JMP .bb_main.1 
.bb_main.1:
AND RBX, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RBX], -45 
AND RCX, 0b111111111111 # instrumentation
CMOVNL RCX, qword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RSI], EDX 
LOOPNE .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RBX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RBX], 0b1000000000000000000000000000000 # instrumentation
BSF RAX, qword ptr [R14 + RBX] 
AND AL, 94 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVNP RCX, qword ptr [R14 + RSI] 
JBE .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RAX, 0b111111111111 # instrumentation
AND CX, word ptr [R14 + RAX] 
AND RDI, 0b111111111111 # instrumentation
CMOVO EBX, dword ptr [R14 + RDI] 
AND RDX, 0b111111111111 # instrumentation
OR DL, byte ptr [R14 + RDX] 
AND RDX, 0b111111111111 # instrumentation
OR DL, byte ptr [R14 + RDX] 
AND RDI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDI], 0b1000000000000000000000000000000 # instrumentation
BSF ESI, dword ptr [R14 + RDI] 
AND CL, 94 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVL RSI, qword ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
AND word ptr [R14 + RBX], -12 
LOOPNE .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 20] # instrumentation
