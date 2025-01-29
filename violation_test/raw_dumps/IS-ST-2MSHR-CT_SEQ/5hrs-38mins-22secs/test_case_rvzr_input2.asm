.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RAX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RAX], RCX 
AND RDX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RDX], CL 
JMP .bb_main.1 
.bb_main.1:
AND RAX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RAX], RSI 
JMP .bb_main.2 
.bb_main.2:
AND RDX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RDX], -25 
AND RCX, 0b111111111111 # instrumentation
CMOVB ECX, dword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
AND word ptr [R14 + RSI], CX 
AND RAX, 0b111111111111 # instrumentation
CMOVNLE SI, word ptr [R14 + RAX] 
AND RAX, 0b111111111111 # instrumentation
AND SIL, byte ptr [R14 + RAX] 
JMP .bb_main.3 
.bb_main.3:
AND AL, -116 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVLE EDX, dword ptr [R14 + RSI] 
AND RBX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RBX], 12 
AND RSI, 0b111111111111 # instrumentation
CMOVS EDX, dword ptr [R14 + RSI] 
AND RDI, 0b111111111111 # instrumentation
XOR word ptr [R14 + RDI], AX 
AND RSI, 0b111111111111 # instrumentation
OR byte ptr [R14 + RSI], 14 
JMP .bb_main.4 
.bb_main.4:
AND RAX, 0b111111111111 # instrumentation
AND qword ptr [R14 + RAX], RCX 
AND RBX, 0b111111111111 # instrumentation
AND dword ptr [R14 + RBX], EDI 
AND RDX, 0b111111111111 # instrumentation
CMOVNLE SI, word ptr [R14 + RDX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
