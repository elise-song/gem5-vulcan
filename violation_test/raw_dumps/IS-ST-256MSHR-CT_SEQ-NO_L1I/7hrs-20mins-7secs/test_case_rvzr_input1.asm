.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RAX, 0b111111111111 # instrumentation
LOCK XOR word ptr [R14 + RAX], 112 
AND RSI, 0b111111111111 # instrumentation
OR byte ptr [R14 + RSI], DL 
AND RDI, 0b111111111111 # instrumentation
AND qword ptr [R14 + RDI], RAX 
JMP .bb_main.1 
.bb_main.1:
AND CL, 77 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVO AX, word ptr [R14 + RCX] 
AND RDX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RDX], DX 
AND RAX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RAX], ESI 
LOOPE .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND CL, 7 # instrumentation
AND RDI, 0b111111111111 # instrumentation
CMOVNS RBX, qword ptr [R14 + RDI] 
JNBE .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND AL, -97 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVLE EBX, dword ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
CMOVO AX, word ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RSI], RSI 
AND RDI, 0b111111111111 # instrumentation
CMOVS RBX, qword ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
CMOVNP DX, word ptr [R14 + RDI] 
AND RAX, 0b111111111111 # instrumentation
AND word ptr [R14 + RAX], SI 
AND RAX, 0b111111111111 # instrumentation
LOCK AND dword ptr [R14 + RAX], EDX 
JO .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RDX, 0b111111111111 # instrumentation
OR EAX, dword ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
CMOVNO SI, word ptr [R14 + RBX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
