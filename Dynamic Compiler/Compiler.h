#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <vector>
#include <unordered_map>

#include "Machine_Codes.h"

#define COMPILER_VERSION "1.0.4"
#define WORD_VARIANT 0x40
#define DEBUG_PROGRAM -1
#define NON_EXECUTABLE -2
#define COMPILER_ERRORS -3
#define DEFAULT_STACK_SIZE 0xFF
#define BYTE_OP_SIZE 0x7F
#define PAGE_SIZE 1 << 12


unsigned char two_complement_8(unsigned char id);
unsigned two_complement_32(unsigned id);

enum RETURN_TYPES { VOID_RETURN, CONST_RETURN, LOCAL_RETURN };
enum REGISTERS : unsigned char { EAX = 0x85, EBX = 0x9D, ECX = 0x8D, EDX = 0x95, ESI = 0xB5, EDI = 0xBD };
enum LOAD_TYPES { LOCAL_LOAD, CONST_LOAD, REG_LOAD };
enum ARG_TYPES { AOT_LOCAL, AOT_INT, AOT_MEMORY, AOT_FLOAT, AOT_DOUBLE, AOT_REG };
enum VAR_TYPES { _INT, _FLOAT, _DOUBLE };
enum FPU_REGISTERS { ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7 };

typedef int (*FuncPtr)();
typedef unsigned char byte;

union funcptr {
  FuncPtr call;
  byte* load;
};

class ARG
{
public:
  ARG(int num) : num_(num), type(AOT_INT){};
  ARG(bool test) : num_(static_cast<int>(test)), type(AOT_INT){};
  ARG(REGISTERS reg) : reg_(reg), type(AOT_REG){};
  ARG(const char* var) : var_(var), type(AOT_LOCAL){};
  ARG(float flt) : flt_(flt), type(AOT_FLOAT){};
  ARG(double dec) : dec_(dec), type(AOT_DOUBLE){};
  ARG(void* mem) : mem_((unsigned)mem), type(AOT_MEMORY){};
  union{
    unsigned mem_;
    int num_;
    float flt_;
    double dec_;
    REGISTERS reg_;
  };
  std::string var_;
  ARG_TYPES type;
private:
  ARG();
};

class AOT_Compiler
{
public:
  //==========================
  // Compiler Functionallity
  //==========================
  AOT_Compiler(const char* name, bool debug = false);
  //Creating Functions.
  void Start_Function(const char* name = "main");
  void End_Function();
  //Creating Objects.
  void Start_Class(std::string name);
  void End_Class();
  //Execution
  int Execute();
  //Compiler info
  const char* Get_Version();

  //==========================
  // Assembler Functionallity
  //==========================

  //--------------------------
  // Abstract x86 
  //--------------------------
  //System Functions
  void Add_Variable(const char* name, VAR_TYPES type = _INT);
  void Make_Label(const char* name);
  void Allocate_Stack(unsigned bytes = 0);
  //System calls
  void Print(ARG argument);
  void Read(std::string name);
  //Stack Operations
  void Push(ARG argument);
  void Push_Adr(std::string name);
  void Pop(unsigned bytes = 0);
  //By name addressing
  void Load_Local(std::string name, ARG argument);
  //Register Addressing
  void Load_Register(REGISTERS reg, ARG argument);
  //Instructions
  void Xchg_Register(REGISTERS dest, REGISTERS source);
  void Add(REGISTERS dest, REGISTERS source);
  void Sub(REGISTERS dest, REGISTERS source);
  void Mul(REGISTERS dest, REGISTERS source);
  void Dec(REGISTERS dest);
  void Inc(REGISTERS dest);
  void Cmp(ARG argument, ARG argument2);
  void Jmp(const char* label);
  void Je(const char* label);
  void Jne(const char* label);
  void Jle(const char* label);
  void Jl(const char* label);
  void Jge(const char* label);
  void Jg(const char* label);
  void Call(void* function);
  void Return(ARG argument = 0);

  //--------------------------
  // Abstract x87
  //--------------------------
  //Stack Operations
  void FPU_Load(ARG argument);
  void FPU_Store(std::string name);
  //Pop instructions
  void FPU_Add(FPU_REGISTERS reg = ST1);
  void FPU_Sub(FPU_REGISTERS reg = ST1);
  void FPU_Mul(FPU_REGISTERS reg = ST1);
  void FPU_Div(FPU_REGISTERS reg = ST1);
  //Non Pop ST(0) Instructions
  void FPU_Sqr();
  void FPU_Abs();
  void FPU_Root();
  //Special x86 extensions
  void FPU_Xchg();
  void FPU_Invert();
  void FPU_Neg();
  void FPU_Round();
  //Special Constants
  void FPU_PI();
  void FPU_Zero();
  void FPU_One();
  //Trig Functions
  void FPU_Sin();
  void FPU_Cos();

private:
  //==========================
  // Compiler Components
  //==========================
  enum COMPILER_ERR { N_STACK };
  struct AOT_Var
  {
    std::string name;
    unsigned adr;
    byte* loc;
    VAR_TYPES type;
  };
  struct LINKER_Var
  {
    std::string name;
    double dec;
    float flt;
  };
  //Simple x86 register helper functions
  unsigned char Reg_to_Reg(REGISTERS dest, REGISTERS source);
  unsigned Register_Index(REGISTERS reg);
  void Move_Register(REGISTERS dest, REGISTERS source);  
  void Load_Local_Mem(unsigned address, unsigned value, LOAD_TYPES type = CONST_LOAD);
  void Load_Local_Mem(unsigned address, REGISTERS reg = EAX);
  //Addressing functions used for AOT execution mode.
  int String_Address(std::string& str);
  int Double_Address(double dec);
  int Float_Address(float dec);
  int Local_Address(std::string& name);
  AOT_Var* Local_Direct(std::string& name);
  //Linker functions
  void Label_Management(const char* label);
  int Add_Double(double dec);
  int Add_Float(float dec);
  int Add_String(std::string&  str);

  //Compiler Information Crawl
  //By name locals
  std::vector<AOT_Var> bnl;
  //Compiler lable set
  std::vector<AOT_Var> cls;
  //Compiler jump list
  std::vector<AOT_Var> cjl;

  
  //==========================
  // Linker Components
  //==========================
  //Linker Information Crawl
  //Executable string pool
  std::vector<LINKER_Var> esp;
  //Executable float pool
  std::vector<LINKER_Var> efp;
  //Executable double pool
  std::vector<LINKER_Var> edp;
  //DATA Section mapping
  unsigned d_section_adr;
  //Lookup table for function linking
  std::unordered_map<const char*, byte*> func_loc;

  //==========================
  // State Management
  //==========================
  byte* buf;
  byte debug_buf[1000];
  byte* p;
  funcptr program;
  unsigned stack_size;
  unsigned pushed_bytes;
  const char* name_;
  bool debug_;
  bool executable_;
  bool stack_allocated;
  bool compiler_error;
};