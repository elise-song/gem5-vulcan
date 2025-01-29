.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RCX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RCX], AL 
AND RAX, 0b111111111111 # instrumentation
CMOVNP EBX, dword ptr [R14 + RAX] 
JMP .bb_main.1 
.bb_main.1:
AND BL, 67 # instrumentation
AND RSI, 0b111111111111 # instrumentation
CMOVBE RAX, qword ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
XOR CL, byte ptr [R14 + RAX] 
AND RCX, 0b111111111111 # instrumentation
XOR AX, word ptr [R14 + RCX] 
AND RCX, 0b111111111111 # instrumentation
CMOVLE RAX, qword ptr [R14 + RCX] 
JP .bb_main.2 
JMP .bb_main.3 
.bb_main.2:
AND DL, 49 # instrumentation
AND RBX, 0b111111111111 # instrumentation
CMOVO DI, word ptr [R14 + RBX] 
AND RSI, 0b111111111111 # instrumentation
AND qword ptr [R14 + RSI], -72 
AND RDX, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RDX], -117 
AND RAX, 0b111111111111 # instrumentation
CMOVNS EAX, dword ptr [R14 + RAX] 
JMP .bb_main.3 
.bb_main.3:
AND RDX, 0b111111111111 # instrumentation
LOCK OR byte ptr [R14 + RDX], CL 
AND RSI, 0b111111111111 # instrumentation
CMOVNL RSI, qword ptr [R14 + RSI] 
AND RAX, 0b111111111111 # instrumentation
OR word ptr [R14 + RAX], 0b1000000000000000 # instrumentation
BSF CX, word ptr [R14 + RAX] 
AND BL, 110 # instrumentation
AND RCX, 0b111111111111 # instrumentation
CMOVNO CX, word ptr [R14 + RCX] 
JO .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RSI, 0b111111111111 # instrumentation
LOCK XOR qword ptr [R14 + RSI], RAX 
AND RDX, 0b111111111111 # instrumentation
LOCK AND byte ptr [R14 + RDX], AL 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
