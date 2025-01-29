.intel_syntax noprefix
LEA R14, [R14 + 40] # instrumentation
MFENCE # instrumentation
.test_case_enter:
.function_main:
.bb_main.entry:
JMP .bb_main.0 
.bb_main.0:
AND AL, -77 # instrumentation
AND RAX, 0b111111111111 # instrumentation
CMOVNLE RDI, qword ptr [R14 + RAX] 
AND RSI, 0b111111111111 # instrumentation
AND qword ptr [R14 + RSI], -3 
JMP .bb_main.1 
.bb_main.1:
AND RSI, 0b111111111111 # instrumentation
OR RSI, qword ptr [R14 + RSI] 
AND RCX, 0b111111111111 # instrumentation
XOR byte ptr [R14 + RCX], SIL 
AND RCX, 0b111111111111 # instrumentation
CMOVNL BX, word ptr [R14 + RCX] 
AND RSI, 0b111111111111 # instrumentation
CMOVNLE EAX, dword ptr [R14 + RSI] 
JL .bb_main.2 
JMP .bb_main.exit 
.bb_main.2:
AND RDX, 0b111111111111 # instrumentation
OR DI, word ptr [R14 + RDX] 
AND RCX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RCX], SI 
AND RCX, 0b111111111111 # instrumentation
OR qword ptr [R14 + RCX], RSI 
LOOPNE .bb_main.3 
JMP .bb_main.exit 
.bb_main.3:
AND RCX, 0b111111111111 # instrumentation
XOR EDX, dword ptr [R14 + RCX] 
AND RBX, 0b111111111111 # instrumentation
OR ECX, dword ptr [R14 + RBX] 
JNLE .bb_main.4 
JMP .bb_main.exit 
.bb_main.4:
AND RDI, 0b111111111111 # instrumentation
LOCK OR dword ptr [R14 + RDI], -55 
AND RDX, 0b111111111111 # instrumentation
OR byte ptr [R14 + RDX], DIL 
AND RAX, 0b111111111111 # instrumentation
XOR qword ptr [R14 + RAX], -61 
AND RSI, 0b111111111111 # instrumentation
LOCK OR qword ptr [R14 + RSI], -6 
AND RAX, 0b111111111111 # instrumentation
LOCK OR word ptr [R14 + RAX], -80 
.bb_main.exit:
.test_case_exit:
MFENCE # instrumentation
LEA R14, [R14 - 40] # instrumentation
