.intel_syntax noprefix
LEA R14, [R14 + 0] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RSI, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RSI], 114 
AND RCX, 0b111111111111 # instrumentation
CMOVO ESI, dword ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
NOT word ptr [R14 + RCX] 
JNS .bb_main.1 
JMP .bb_main.exit 
.bb_main.1:
AND BL, 46 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVNZ AX, word ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RDX], AL 
AND RDX, 0b111111111111 # instrumentation
CMOVP RSI, qword ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
AND CX, word ptr [R14 + RAX] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RAX], 33 
JNL .bb_main.2 
JMP .bb_main.4 
.bb_main.2:
AND AL, -122 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNLE RDX, qword ptr [R14 + RDX] 
AND RDI, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RDI], 107 
AND RAX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RAX], -98 
JNBE .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
JMP .bb_main.4 
.bb_main.4:
AND AL, -34 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVNB EDX, dword ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
OR EAX, dword ptr [R14 + RAX] 
AND RAX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RAX], 0b1000000000000000000000000000000 # instrumentation
BSR RSI, qword ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
OR SIL, byte ptr [R14 + RBX] 
AND RCX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RCX], -29 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 0] # instrumentation
