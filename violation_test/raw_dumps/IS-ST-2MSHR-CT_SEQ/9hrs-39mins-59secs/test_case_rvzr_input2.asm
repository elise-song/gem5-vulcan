.intel_syntax noprefix
LEA R14, [R14 + 44] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RDX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RDX] 
AND RDI, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RDI], SI 
AND RDX, 0b111111111111 # instrumentation
OR word ptr [R14 + RDX], BX 
AND RDX, 0b111111111111 # instrumentation
CMOVBE SI, word ptr [R14 + RDX] 
JMP .bb_main.1 
.bb_main.1:
AND RDX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RDX], -111 
AND RDX, 0b111111111111 # instrumentation
OR word ptr [R14 + RDX], 0b1000000000000000 # instrumentation
BSR CX, word ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
AND byte ptr [R14 + RSI], DIL 
JLE .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND AL, 74 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVL EDI, dword ptr [R14 + RAX] 
LOOPNE .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND DL, 91 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVP RDI, qword ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RDI], 0b1000000000000000000000000000000 # instrumentation
BSF EAX, dword ptr [R14 + RDI] 
AND RCX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RCX], DL 
JMP .bb_main.4 
.bb_main.4:
AND RDI, 0b111111111111 # instrumentation
LOCK AND qword ptr [R14 + RDI], RBX 
AND RCX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RCX], RDX 
AND RBX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RBX], DL 
AND RCX, 0b111111111111 # instrumentation
CMOVS DX, word ptr [R14 + RCX] 
AND RDI, 0b111111111111 # instrumentation
LOCK NOT word ptr [R14 + RDI] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 44] # instrumentation
