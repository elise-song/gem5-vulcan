.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
OR SIL, byte ptr [R14 + RDX] 
AND RDI, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RDI], -109 
LOOPNE .bb_main.1 
JMP .bb_main.2 
.bb_main.1:
AND RDX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RDX] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RAX], DIL 
AND RAX, 0b111111111111 # instrumentation
CMOVLE RAX, qword ptr [R14 + RAX] 
AND RDX, 0b111111111111 # instrumentation
NOT qword ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
CMOVP RDI, qword ptr [R14 + RBX] 
AND RDX, 0b111111111111 # instrumentation
XOR BL, byte ptr [R14 + RDX] 
LOOPE .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND BL, 91 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVNS EBX, dword ptr [R14 + RBX] 
JMP .bb_main.3 
.bb_main.3:
AND RCX, 0b111111111111 # instrumentation
OR word ptr [R14 + RCX], 95 
AND RDX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RDX], 86 
AND RCX, 0b111111111111 # instrumentation
CMOVNBE AX, word ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RDX], RBX 
AND RDX, 0b111111111111 # instrumentation
AND SIL, byte ptr [R14 + RDX] 
JMP .bb_main.4 
.bb_main.4:
AND RAX, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RAX], 2106851056 
AND RCX, 0b111111111111 # instrumentation
CMOVNZ EAX, dword ptr [R14 + RCX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
