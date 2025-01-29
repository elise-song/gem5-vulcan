.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RBX, 0b111111111111 # instrumentation
NOT dword ptr [R14 + RBX] 
AND RAX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RAX], CL 
AND RAX, 0b111111111111 # instrumentation
OR word ptr [R14 + RAX], 58 
JL .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND RAX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RAX], -69 
AND RDX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RDX], RSI 
AND RCX, 0b111111111111 # instrumentation
NOT byte ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
CMOVNB EAX, dword ptr [R14 + RDI] 
JMP .bb_main.2 
.bb_main.2:
AND RCX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RCX], -127 
AND RAX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RAX], RDX 
AND RCX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RCX], -86 
AND RAX, 0b111111111111 # instrumentation
OR RDX, qword ptr [R14 + RAX] 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR dword ptr [R14 + RBX], EDI 
JMP .bb_main.3 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
OR RDI, qword ptr [R14 + RBX] 
JNBE .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RSI], 16 
AND RSI, 0b111111111111 # instrumentation
OR EDX, dword ptr [R14 + RSI] 
AND RDX, 0b111111111111 # instrumentation
OR EDI, dword ptr [R14 + RDX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
