//x86
#define MOV 0x8B
#define RETURN 0xC3
#define FAR_CALL 0xFF
#define STK_POP 0x83
#define MEM_LOD 0x89

//Register ops
#define REG_INC 0x40
#define REG_DEC 0x48
#define REG_PSH 0x50
#define PSH_EDX 0x52
#define PSH_EBP 0x55
#define POP_EDX 0x5A
#define POP_EBP 0x5D
#define EAX_XCH 0x90
#define REG_LOD 0xB8
#define ADD_ESP 0xC4
#define SUB_ESP 0xEC

//Jump Codes
#define CODE_JMP 0xEB
#define CODE_JE 0x74
#define CODE_JNE 0x75
#define CODE_JLE 0x7E
#define CODE_JL 0x7C
#define CODE_JG 0x7F

//x87
#define FPU_LOAD_ADDR 0x05
#define FPU_LOAD_VAR 0x45
#define FPU_PUSH 0x5D
#define FPU_MATH 0xDE
#define FPU_FLOAT_OP 0xD9
#define FPU_INT_OP 0xDB
#define FPU_DOUBLE_OP 0xDD