.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND BL, -14 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVLE BX, word ptr [R14 + RCX] 
AND RAX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RAX], -89 
JB .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND AL, 81 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVO RDX, qword ptr [R14 + RBX] 
AND RBX, 0b111111111111 # instrumentation
CMOVNP CX, word ptr [R14 + RBX] 
AND RDI, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RDI], 1795341934 
LOOP .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND RDX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RDX], SIL 
AND RCX, 0b111111111111 # instrumentation
CMOVLE ESI, dword ptr [R14 + RCX] 
JS .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RDX, 0b111111111111 # instrumentation
OR word ptr [R14 + RDX], 0b1000000000000000 # instrumentation
BSR DX, word ptr [R14 + RDX] 
AND AL, -61 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNO RAX, qword ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RCX], RCX 
AND RSI, 0b111111111111 # instrumentation
OR CX, word ptr [R14 + RSI] 
AND RDX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RDX], 8 
JBE .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND AL, -123 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVNLE EAX, dword ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RDX], DL 
AND RDX, 0b111111111111 # instrumentation
OR DL, byte ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RBX], RCX 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
