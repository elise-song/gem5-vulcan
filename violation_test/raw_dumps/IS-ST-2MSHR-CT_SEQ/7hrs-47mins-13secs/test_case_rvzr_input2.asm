.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND DL, -51 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNBE BX, word ptr [R14 + RAX] 
JNS .bb_main.1 
JMP .bb_main.4 
.bb_main.1:
AND RDI, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RDI], 98 
AND RCX, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RCX], EBX 
AND RCX, 0b111111111111 # instrumentation
AND CL, byte ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RSI], -82 
JL .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND RDX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RDX], CL 
AND RCX, 0b111111111111 # instrumentation
CMOVLE DX, word ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RSI], AL 
AND RCX, 0b111111111111 # instrumentation
CMOVP EDX, dword ptr [R14 + RCX] 
JBE .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RCX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RCX], 17 
AND RBX, 0b111111111111 # instrumentation
CMOVNS EDX, dword ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
CMOVBE AX, word ptr [R14 + RDX] 
LOOPNE .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RSI], -89 
AND RCX, 0b111111111111 # instrumentation
CMOVB RBX, qword ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RDI], 45 
AND RDX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDX], BL 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
