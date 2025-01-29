.intel_syntax noprefix
LEA R14, [R14 + 60] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND RSI, 0b111111111111 # instrumentation
OR dword ptr [R14 + RSI], -16 
AND RBX, 0b111111111111 # instrumentation
LOCK XOR byte ptr [R14 + RBX], DL 
JBE .bb_main.1 
JMP .bb_main.3 
.bb_main.1:
AND RDX, 0b111111111111 # instrumentation
LOCK AND word ptr [R14 + RDX], SI 
AND RDX, 0b111111111111 # instrumentation
LOCK NOT qword ptr [R14 + RDX] 
JP .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND AL, 12 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVO RAX, qword ptr [R14 + RDX] 
AND RBX, 0b111111111111 # instrumentation
CMOVBE EDI, dword ptr [R14 + RBX] 
AND RCX, 0b111111111111 # instrumentation
XOR word ptr [R14 + RCX], BX 
AND RAX, 0b111111111111 # instrumentation
CMOVNL RDI, qword ptr [R14 + RAX] 
JMP .bb_main.3 
.bb_main.3:
AND RAX, 0b111111111111 # instrumentation
XOR dword ptr [R14 + RAX], 111 
AND RSI, 0b111111111111 # instrumentation
OR qword ptr [R14 + RSI], RDI 
AND RDI, 0b111111111111 # instrumentation
XOR AL, byte ptr [R14 + RDI] 
AND RSI, 0b111111111111 # instrumentation
CMOVNZ ECX, dword ptr [R14 + RSI] 
AND RSI, 0b111111111111 # instrumentation
XOR EBX, dword ptr [R14 + RSI] 
JMP .bb_main.4 
.bb_main.4:
AND AL, 17 # instrumentation
AND RDX, 0b111111111111 # instrumentation
CMOVNS RCX, qword ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
NOT qword ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
CMOVLE EAX, dword ptr [R14 + RBX] 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 60] # instrumentation
