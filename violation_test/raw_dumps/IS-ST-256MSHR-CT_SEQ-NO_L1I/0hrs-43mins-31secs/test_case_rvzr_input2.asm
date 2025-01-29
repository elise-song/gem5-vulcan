.intel_syntax noprefix
LEA R14, [R14 + 52] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
OR DI, word ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RAX], 0b1000000000000000000000000000000 # instrumentation
BSF RBX, qword ptr [R14 + RAX] 
AND DIL, 74 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVL DX, word ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RDX], RBX 
JP .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND RDI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDI], EDI 
LOOP .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RDI, 0b111111111111 # instrumentation
AND dword ptr [R14 + RDI], -16 
AND RSI, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RSI], -1675482471 
AND RDI, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RDI], 175742188 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RSI], AL 
AND RAX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RAX], AX 
AND RBX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RBX], DI 
JZ .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RSI, 0b111111111111 # instrumentation
AND word ptr [R14 + RSI], -35 
AND RAX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RAX], -108 
JL .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RDX, 0b111111111111 # instrumentation
OR word ptr [R14 + RDX], 0b1000000000000000 # instrumentation
BSR SI, word ptr [R14 + RDX] 
AND DIL, -54 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVLE EAX, dword ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RBX], SI 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 52] # instrumentation
