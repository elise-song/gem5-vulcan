.intel_syntax noprefix
LEA R14, [R14 + 20] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
NOT dword ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RBX], 110 
JNBE .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND DL, 4 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVB EAX, dword ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RSI], -103 
JNL .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND DL, 108 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVB RDX, qword ptr [R14 + RBX] 
JNBE .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND DL, -75 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVZ DX, word ptr [R14 + RBX] 
AND RDI, 0b111111111111 # instrumentation
AND RCX, qword ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RCX], ECX 
AND RBX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RBX], BX 
AND RBX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RBX], 42 
AND RDI, 0b111111111111 # instrumentation
LOCK NOT byte ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RSI], RDX 
AND RAX, 0b111111111111 # instrumentation
CMOVBE BX, word ptr [R14 + RAX] 
JRCXZ .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RDX, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RDX], 1408812779 
AND RDX, 0b111111111111 # instrumentation
XOR RCX, qword ptr [R14 + RDX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 20] # instrumentation
