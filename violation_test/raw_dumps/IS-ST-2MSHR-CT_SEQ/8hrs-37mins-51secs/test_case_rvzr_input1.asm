.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND AL, -103 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVLE RCX, qword ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
CMOVBE SI, word ptr [R14 + RSI] 
AND RDI, 0b111111111111 # instrumentation
CMOVNLE DI, word ptr [R14 + RDI] 
AND RDI, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RDI], AL 
AND RAX, 0b111111111111 # instrumentation
TEST word ptr [R14 + RAX], CX 
JMP .bb_main.1 
.bb_main.1:
AND RCX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RCX], 0b1000000000000000000000000000000 # instrumentation
BSF RCX, qword ptr [R14 + RCX] 
AND AL, 46 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVS CX, word ptr [R14 + RBX] 
JBE .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND BL, -76 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVB ESI, dword ptr [R14 + RDX] 
AND RSI, 0b111111111111 # instrumentation
TEST qword ptr [R14 + RSI], RCX 
LOOP .bb_main.3 
JMP .bb_main.4 
.bb_main.3:
AND RBX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RBX], DIL 
AND RDI, 0b111111111111 # instrumentation
TEST dword ptr [R14 + RDI], 543566554 
AND RAX, 0b111111111111 # instrumentation
AND byte ptr [R14 + RAX], CL 
JMP .bb_main.4 
.bb_main.4:
AND DL, -36 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVNZ EDI, dword ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
CMOVNBE ECX, dword ptr [R14 + RSI] 
AND RBX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RBX], AL 
AND RAX, 0b111111111111 # instrumentation
AND word ptr [R14 + RAX], SI 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
