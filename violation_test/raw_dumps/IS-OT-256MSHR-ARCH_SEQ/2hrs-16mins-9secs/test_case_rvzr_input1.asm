.intel_syntax noprefix
LEA R14, [R14 + 44] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RDX], BL 
JNO .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RDI, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RDI], -120 
AND RDX, 0b111111111111 # instrumentation
XOR RDI, qword ptr [R14 + RDX] 
JP .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND RSI, 0b111111111111 # instrumentation
OR AX, word ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
OR AX, word ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RBX], 0b1000000000000000000000000000000 # instrumentation
BSF EAX, dword ptr [R14 + RBX] 
AND BL, -116 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNS AX, word ptr [R14 + RAX] 
JS .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RBX], AL 
AND RAX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RAX], DL 
AND RDX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RDX], -99 
JMP .bb_main.4 
.bb_main.4:
AND RBX, 0b111111111111 # instrumentation
OR RSI, qword ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RAX], RDX 
AND RCX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RCX], AL 
AND RCX, 0b111111111111 # instrumentation
OR dword ptr [R14 + RCX], -48 
AND RDX, 0b111111111111 # instrumentation
CMOVB AX, word ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RBX], EDX 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 44] # instrumentation
