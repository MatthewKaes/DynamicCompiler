#include "Compiler.h"
#pragma warning(disable: 4996)

unsigned char two_complement_8(unsigned char id)
{
  return (0xFF - id) + 1;
}
unsigned two_complement_32(unsigned id)
{
  return (0xFFFFFFFF - id) + 1;
}

AOT_Compiler::AOT_Compiler(const char* name, bool debug) : debug_(debug), stack_size(0), d_section_adr(0),
                                                           executable_(false), name_(name), pushed_bytes(0),
                                                           stack_allocated(false), compiler_error(false)
{
  buf = (byte*)VirtualAllocEx( GetCurrentProcess(), 0, 1<<16, MEM_COMMIT, PAGE_EXECUTE_READWRITE );
  if(debug)
  {
    p = debug_buf;
  }
  else
  {
    p = buf;
    program.load = buf;
  }
  //Obviously since these are used in direct addressing
  //we can't have them go and reallocating themselves.
  //When they grow we will get a copy and we don't want that.
  esp.reserve(DEFAULT_STACK_SIZE);
  efp.reserve(DEFAULT_STACK_SIZE);
  edp.reserve(DEFAULT_STACK_SIZE);
}
void AOT_Compiler::Start_Function()
{
  //Block execution while program is being worked on.
  executable_ = false;
  stack_allocated = false;
  *p++ = 0x52; // push edx
  *p++ = 0x55; //push ebp

  *p++ = 0x8B;
  *p++ = 0xEC; //mov ebp, esp
}
void AOT_Compiler::Add_Variable(const char* name, VAR_TYPES type)
{
  AOT_Var var;
  var.name.assign(name);
  var.adr = stack_size;
  var.type = type;
  switch(type)
  {
  case _INT:
    stack_size += sizeof(int);
    break;
  case _FLOAT:
    stack_size += sizeof(float);
    break;
  case _DOUBLE:
    stack_size += sizeof(double);
    break;
  }
  bnl.push_back(var);
}
void AOT_Compiler::Make_Label(const char* name)
{
  AOT_Var label;
  label.name.assign(name);
  label.adr = (unsigned)p;
  cls.push_back(label);
  std::vector<AOT_Var>::iterator walker = cjl.begin();
  //search backwards for foward jumps
  while(walker != cjl.end())
  {
    if(!walker->name.compare(name))
    {
      unsigned distance = label.adr - walker->adr - 1;
      (walker->loc)[0] = distance;
      cjl.erase(walker);
      walker = cjl.begin();
    }
    else
    {
      walker++;
    }
  }
}
void AOT_Compiler::Cmp(ARG argument, ARG argument2)
{
  switch(argument.type)
  {
  case AOT_LOCAL:
    switch(argument2.type)
    {
      case AOT_LOCAL:
        Load_Register(EAX, argument2);
        *p++ = 0x39;
        *p++ = 0x45;
        *p++ = two_complement_8(Local_Address(argument.var_));
        break;
      case AOT_INT:
        if(argument.num_ > 0x7F)
        {
          *p++ = 0x83;
          *p++ = 0x7D;
          *p++ = two_complement_8(Local_Address(argument.var_));
          *p++ = (unsigned char)argument2.num_;
        }
        else
        {
          *p++ = 0x81;
          *p++ = 0x7D;
          *p++ = two_complement_8(Local_Address(argument.var_));
          (int&)p[0] = argument2.num_; p+= sizeof(int);
        }
    }
    break;
  case AOT_INT:
    switch(argument2.type)
    {
      case AOT_LOCAL:
        if(argument.num_ > 0x7F)
        {
          *p++ = 0x83;
          *p++ = 0x7D;
          *p++ = two_complement_8(Local_Address(argument2.var_));
          *p++ = (unsigned char)argument.num_;
        }
        else
        {
          *p++ = 0x81;
          *p++ = 0x7D;
          *p++ = two_complement_8(Local_Address(argument2.var_));
          (int&)p[0] = argument.num_; p+= sizeof(int);
        }
        break;
      case AOT_INT:
        if(argument.num_ > argument2.num_)
        {
          *p++ = 0xB8;
          (int&)p[0] = 0x1; p+= sizeof(int);
        }
        else
        {
          *p++ = 0xB8;
          (int&)p[0] = 0x0; p+= sizeof(int);
        }
        *p++ = 0x85;
        *p++ = 0xC0;
        break;
    }
    break;
  }
}
void AOT_Compiler::Jmp(const char* label)
{
  *p++ = 0xEB;  
  Label_Management(label);
}
void AOT_Compiler::Je(const char* label)
{
  *p++ = 0x74;
  Label_Management(label);
}
void AOT_Compiler::Jne(const char* label)
{
  *p++ = 0x75;
  Label_Management(label);
}
void AOT_Compiler::Jle(const char* label)
{
  *p++ = 0x7E;
  Label_Management(label);
}
void AOT_Compiler::Jl(const char* label)
{
  *p++ = 0x7C;
  Label_Management(label);
}
void AOT_Compiler::Jg(const char* label)
{
  *p++ = 0x7F;
  Label_Management(label);
}
void AOT_Compiler::Allocate_Stack(unsigned bytes)
{
  stack_size += bytes;
  if(stack_size < 0x7F)
  {
    *p++ = 0x83;
    *p++ = 0xEC;
    *p++ = (unsigned char)(stack_size);
  }
  else
  {
    *p++ = 0x81;
    *p++ = 0xEC; //sub esp
    (int&)p[0] = stack_size; p+= sizeof(int);
  }
  stack_allocated = true;
}  
void AOT_Compiler::Print(ARG argument)
{
  AOT_Var* loc;
  if(argument.type == AOT_LOCAL)
  {
    loc = Local_Direct(argument.var_);
    if(loc->type == _FLOAT)
    {
      //This makes it so variadic part of printf is
      //properly supported.
      FPU_Load(argument);

      //Make room for a double instead of a float.
      *p++ = 0x83;
      *p++ = 0xEC;
      *p++ = 0x08;

      //Load it on the stack as a double
      *p++ = 0xDD;
      *p++ = 0x1C;
      *p++ = 0x24;
      pushed_bytes += 4;
    }
    else
    {
      Push(argument);
    }
  }
  else if(argument.type == AOT_FLOAT)
  {
    Push((double)argument.flt_);
  }
  else
  {
    Push(argument);
  }
  *p++ = 0x68;
  switch(argument.type)
  {
  case AOT_FLOAT:
  case AOT_DOUBLE:
    (int&)p[0] = (int)"%f\n"; p+= sizeof(int);
    break;
  case AOT_LOCAL:
    if(loc->type == _FLOAT || loc->type == _DOUBLE)
    {
      (int&)p[0] = (int)"%f\n"; p+= sizeof(int);
      break;
    }
  case AOT_INT:
  default:
    (int&)p[0] = (int)"%d\n"; p+= sizeof(int);
    break;
  }
  Call(printf);
  Pop();
}
void AOT_Compiler::Read(std::string name)
{
  AOT_Var* loc = Local_Direct(name);
  if(loc != NULL)
  {
    Push_Adr(name);
    switch(loc->type)
    {
    case _INT:
      Push("%d");
      break;
    case _DOUBLE:
    case _FLOAT:
      Push("%f");
      break;
    }
    Call(scanf);
  }
}
void AOT_Compiler::Push(ARG argument)
{
  int address;
  pushed_bytes += 4;
  switch(argument.type)
  {
  case AOT_REG:
    switch(argument.reg_)
    {
    case EAX:
      *p++ = 0x50;
      return;
    case EBX:
      *p++ = 0x53;
      return;
    case ECX:
      *p++ = 0x51;
      return;
    case EDX:
      *p++ = 0x52;
      return;
    }
    break;
  case AOT_LOCAL:
    address = Local_Address(argument.var_);
    if(address != -1)
    {
      AOT_Var* loc = Local_Direct(argument.var_);
      if(loc->type == _FLOAT || loc->type == _DOUBLE)
      {
        FPU_Load(argument);
        
        //Make room
        *p++ = 0x83;
        *p++ = 0xEC;
        
        if(loc->type == _FLOAT)
        {
          *p++ = 0x04;
          *p++ = 0xD9;
        }
        else
        {
          *p++ = 0x08;
          *p++ = 0xDD;
          pushed_bytes += 4;
        }
        *p++ = 0x1C;
        *p++ = 0x24;
      }
      else
      {
        *p++ = 0xFF;
        *p++ = 0x75;
        *p++ = two_complement_8(address + 4);
      }
    }
    else
    {
      *p++ = 0x68;
      (int&)p[0] = (unsigned)(String_Address(argument.var_)); p+= sizeof(int);
    }
    return;
    break;
  case AOT_INT:
    if(abs(argument.num_) < 0x7F)
    {
      *p++ = 0x6A;
      *p++ = (unsigned char)(argument.num_);
    }
    else
    {
      *p++ = 0x68;
      (int&)p[0] = (unsigned)(argument.num_); p+= sizeof(int);
    }
    return;
  case AOT_DOUBLE:

    //Double
    *p++ = 0xDD;
    *p++ = 0x05;
    (int&)p[0] = (int)Double_Address(argument.dec_); p+= sizeof(int);

    //Make room
    *p++ = 0x83;
    *p++ = 0xEC;
    *p++ = 0x08;

    //store on stack
    //double
    *p++ = 0xDD;
    *p++ = 0x1C;
    *p++ = 0x24;
    pushed_bytes += 4;
    return;
  case AOT_FLOAT:
    //Float
    *p++ = 0xD9;
    *p++ = 0x05;
    (int&)p[0] = (int)Float_Address(argument.flt_); p+= sizeof(int);

    //Make room
    *p++ = 0x83;
    *p++ = 0xEC;
    *p++ = 0x04;

    //store on stack
    //Float
    *p++ = 0xD9;
    *p++ = 0x1C;
    *p++ = 0x24;

    //pushed_bytes += 4;
    return;
  }
}
void AOT_Compiler::Push_Adr(std::string name)
{
  int address = Local_Address(name);
  if(address != -1)
  {
    *p++ = 0x8D;
    *p++ = 0x45;
    *p++ = two_complement_8(address + 4);
    *p++ = 0x50;
    pushed_bytes += 4;
  }
}
void AOT_Compiler::Pop(unsigned bytes)
{
  if(!pushed_bytes)
  {
    return;
  }
  if(!bytes)
  {
    *p++ = 0x83;
    *p++ = 0xC4;
    *p++ = (unsigned char)(pushed_bytes);
    pushed_bytes = 0;
  }
  else
  {
    *p++ = 0x83;
    *p++ = 0xC4;
    *p++ = (unsigned char)(bytes);
    pushed_bytes -= bytes;
  }
}
void AOT_Compiler::Load_Local(std::string name, ARG argument)
{
  switch(argument.type)
  {
  case AOT_LOCAL:
    Load_Local_Mem(Local_Address(name), Local_Address(argument.var_), LOCAL_LOAD);
    return;
  case AOT_REG:
    Load_Local_Mem(Local_Address(name), argument.reg_);
    return;
  case AOT_INT:
    Load_Local_Mem(Local_Address(name), argument.num_);
    return;
  case AOT_FLOAT:
    FPU_Load(argument);
    FPU_Store(name);
    return;
  }
}
void AOT_Compiler::Load_Local_Mem(unsigned address, unsigned value, LOAD_TYPES type)
{
  if(type == CONST_LOAD)
  {
    *p++ = 0xC7;
    if(address < 0x7F)
    {
      *p++ = EAX - WORD_VARIANT;
      *p++ = two_complement_8(address + 4);
    }
    else
    {
      *p++ = EAX;
      (int&)p[0] = two_complement_32(address + 4); p+= sizeof(int); //dword ptr [ebp-adr]
    }
    (int&)p[0] = value; p+= sizeof(int);
    //C7 85 CC 5A FF FF 41 00 00 00 mov dword ptr [ebp-0A534h],41h 
  }
  else if(type == LOCAL_LOAD)
  {
    *p++ = 0x8B;
    if(address < 0x7F)
    {
      *p++ = EAX - WORD_VARIANT;
      *p++ = two_complement_8(value + 4);
    }
    else
    {
      *p++ = EAX;
      (int&)p[0] = two_complement_32(value + 4); p+= sizeof(int); //dword ptr [ebp-adr]
    }
    Load_Local_Mem(address);
  }
}
void AOT_Compiler::Load_Local_Mem(unsigned address, REGISTERS reg)
{
  *p++ = 0x89;
  if(address < 0x7F)
  {
    *p++ = reg - WORD_VARIANT;
    *p++ = two_complement_8(address + 4);
  }
  else
  {
    *p++ = reg;
    (int&)p[0] = two_complement_32(address + 4); p+= sizeof(int); //dword ptr [ebp-adr]
  }
}

void AOT_Compiler::Load_Register(REGISTERS reg, ARG argument)
{
  int data;
  switch(argument.type)
  {
  case AOT_REG:
    Move_Register(reg, argument.reg_);
    return;
  case AOT_MEMORY:
  case AOT_INT:
    switch(reg)
    {
    case EAX:
      if(argument.num_ == 0)
      {
        //xor eax, eax
        *p++ = 0x33;
        *p++ = 0xC0;
        return;
      }
      else
      {
        *p++ = 0xB8; // mov eax
      }
      break;
    case EBX:
      *p++ = 0xBB; // mov ebx
      break;
    case ECX:
      *p++ = 0xB9; // mov ecx
      break;
    case EDX:
      *p++ = 0xBA; // mov edx
      break;
    }
    (int&)p[0] = argument.num_; p+= sizeof(int);
    return;
  case AOT_LOCAL:
    *p++ = 0x8B;
    data = Local_Address(argument.var_);
    if(data < 0x7F)
    {
      *p++ = reg - WORD_VARIANT;
      *p++ = two_complement_8(data + 4);
    }
    else
    {
      *p++ = reg; //mov eax,dword ptr[ebp - data - 4]
      (int&)p[0] = two_complement_32(data + 4); p+= sizeof(int);
    }
    return;
  }
}
void AOT_Compiler::Move_Register(REGISTERS dest, REGISTERS source)
{
  *p++ = 0x8B;
  *p++ = Reg_to_Reg(dest, source);
}
void AOT_Compiler::Xchg_Register(REGISTERS dest, REGISTERS source)
{
  if(source == dest)
  {
    //nop
    return;
  }
  //Super fast EAX xchgs
  if(dest == EAX)
  {
    switch(source)
    {
    case EBX:
      *p++ = 0x93;
      break;
    case ECX:
      *p++ = 0x91;
      break;
    case EDX:
      *p++ = 0x92;
    }
  }
  else if(source == EAX)
  {
    switch(dest)
    {
    case EBX:
      *p++ = 0x93;
      break;
    case ECX:
      *p++ = 0x91;
      break;
    case EDX:
      *p++ = 0x92;
    }
  }
  else
  {
    *p++ = 0x87;
    *p++ = Reg_to_Reg(dest, source);
  }
}
void AOT_Compiler::Add(REGISTERS dest, REGISTERS source)
{
  *p++ = 0x03;
  *p++ = Reg_to_Reg(dest, source);
}
void AOT_Compiler::Sub(REGISTERS dest, REGISTERS source)
{
  *p++ = 0x2B;
  *p++ = Reg_to_Reg(dest, source);
}
void AOT_Compiler::Mul(REGISTERS dest, REGISTERS source)
{
  *p++ = 0x0F;
  *p++ = 0xAF;
  *p++ = Reg_to_Reg(dest, source);
}
void AOT_Compiler::Inc(REGISTERS dest)
{
  switch(dest)
  {
  case EAX:
    *p++ = 0x40;
    return;
  case EBX:
    *p++ = 0x43;
    return;
  case ECX:
    *p++ = 0x41;
    return;
  case EDX:
    *p++ = 0x42;
    return;
  }
}
void AOT_Compiler::Dec(REGISTERS dest)
{
  switch(dest)
  {
  case EAX:
    *p++ = 0x48;
    return;
  case EBX:
    *p++ = 0x4B;
    return;
  case ECX:
    *p++ = 0x49;
    return;
  case EDX:
    *p++ = 0x4A;
    return;
  }
}
void AOT_Compiler::Call(void* function)
{
  Load_Register(ECX, function);
  *p++ = 0xFF;
  *p++ = 0xD1;
  Pop();
}
void AOT_Compiler::Return(ARG argument)
{
  //Floats and doubles are returned on the FPU
  //x86 primitives are passed via EAX.
  if(argument.type == AOT_LOCAL)
  {
    AOT_Var* loc = Local_Direct(argument.var_);
    if(loc->type == _FLOAT || loc->type == _DOUBLE)
    {
      FPU_Load(argument);
    }
    else
    {
      Load_Register(EAX, argument);
    }
  }
  if(argument.type == AOT_FLOAT || argument.type == AOT_DOUBLE)
  {
    FPU_Load(argument);
  }
  else
  {
    Load_Register(EAX, argument);
  }
  *p++ = 0x8B;
  *p++ = 0xE5; // mov esp,ebp 
  *p++ = 0x5D; // pop ebp
  *p++ = 0x5A; // pop edx
  *p++ = 0xC3; // ret
}
void AOT_Compiler::End_Function()
{
  Return();
  //Example of compiler errors. Add more later.
  if(!stack_allocated && stack_size > 0)
  {
    printf("ERROR 1: Stack never allocated.\n");
    compiler_error = true;
  }
  executable_ = true;
}
int AOT_Compiler::Execute()
{
  if(compiler_error)
  {
    printf("Execute call on program (%s) with errors.\n", name_);
    return COMPILER_ERRORS;
  }
  else if(debug_)
  {
    printf("Execute call on program (%s) in debug mode.\n", name_);
    return DEBUG_PROGRAM;
  }
  else if(!executable_)
  {
    printf("Execute call on a non executable program (%s).\n", name_);
    return NON_EXECUTABLE;
  }
  else
  {
    return program.call();
  }
}
const char* Get_Version()
{
  return COMPILER_VERSION;
}
unsigned char AOT_Compiler::Reg_to_Reg(REGISTERS dest, REGISTERS source)
{
  unsigned char base;
  switch(dest)
  {
  case EAX:
    base = 0xC0;
    break;
  case EBX:
    base = 0xD8;
    break;
  case ECX:
    base = 0xC8;
    break;
  case EDX:
    base = 0xD0;
    break;
  }
  switch(source)
  {
  case EAX:
    return base; // mov eax
  case EBX:
    return base + 0x03; // mov ebx
  case ECX:
    return base + 0x01; // mov ecx
  case EDX:
    return base + 0x02; // mov edx
  }
  return NULL;
}
int AOT_Compiler::String_Address(std::string& str)
{
  int address;
  for(unsigned i = 0; i < esp.size(); i++)
  {
    if(!esp[i].name.compare(str))
    {
      address = (int)esp[i].name.c_str();
      return address;
    }
  }
  return Add_String(str);
}
int AOT_Compiler::Add_String(std::string& str)
{
  LINKER_Var var;
  var.name.assign(str);
  esp.push_back(var);
  return (int)(esp.back().name.c_str());
}
void AOT_Compiler::Label_Management(const char* label)
{
  std::vector<AOT_Var>::iterator walker;
  //search lables for backwards jumps
  for(walker = cls.begin(); walker != cls.end(); walker++)
  {
    if(!walker->name.compare(label))
    {
      *p++ = two_complement_8((unsigned)p - walker->adr + 1);
      return;
    }
  }
  //didn't find a matching label
  AOT_Var var;
  var.name.assign(label);
  var.adr = (unsigned)p;
  var.loc = p;
  cjl.push_back(var);
  p++;
}
int AOT_Compiler::Float_Address(float dec)
{
  for(unsigned i = 0; i < efp.size(); i++)
  {
    if(efp[i].flt == dec)
    {
      return (int)&(efp[i].flt);
    }
  }
  return Add_Float(dec);
}
int AOT_Compiler::Add_Float(float dec)
{
  LINKER_Var var;
  var.flt = dec;
  efp.push_back(var);
  return (int)&(efp.back().flt);
}
int AOT_Compiler::Double_Address(double dec)
{
  for(unsigned i = 0; i < edp.size(); i++)
  {
    if(edp[i].dec == dec)
    {
      return (int)&(edp[i].dec);
    }
  }
  return Add_Double(dec);
}
int AOT_Compiler::Add_Double(double dec)
{
  LINKER_Var var;
  var.dec = dec;
  edp.push_back(var);
  return (int)&(edp.back().dec);
}
int AOT_Compiler::Local_Address(std::string& name)
{
  for(unsigned i = 0; i < bnl.size(); i++)
  {
    if(!bnl[i].name.compare(name))
    {
      return bnl[i].adr;
    }
  }
  return -1;
}
AOT_Compiler::AOT_Var* AOT_Compiler::Local_Direct(std::string& name)
{
  for(unsigned i = 0; i < bnl.size(); i++)
  {
    if(!bnl[i].name.compare(name))
    {
      return &(bnl[i]);
    }
  }
  return NULL;
}
void AOT_Compiler::FPU_Load(ARG argument)
{
  AOT_Var* var;
  switch(argument.type)
  {
  case AOT_LOCAL:
    var = Local_Direct(argument.var_);
    switch(var->type)
    {
    case _FLOAT:
      *p++ = 0xD9;
      *p++ = 0x45;
      *p++ = two_complement_8(Local_Address(argument.var_) + 4);
      return;
    case _INT:
      *p++ = 0xDB;
      *p++ = 0x45;
      *p++ = two_complement_8(Local_Address(argument.var_) + 4);
      return;
    case _DOUBLE:
      *p++ = 0xDD;
      *p++ = 0x45;
      *p++ = two_complement_8(Local_Address(argument.var_) + 4);
      return;
    }
  case AOT_FLOAT:    
    *p++ = 0xD9;
    *p++ = 0x05;
    (int&)p[0] = (int)Float_Address(argument.flt_); p+= sizeof(int);
    return;
  case AOT_DOUBLE:    
    *p++ = 0xDD;
    *p++ = 0x05;
    (int&)p[0] = (int)Double_Address(argument.dec_); p+= sizeof(int);
    return;
    //FPU can't actually load integer values
    //so we need to load it as an "address":
  case AOT_INT:    
    *p++ = 0xD9;
    *p++ = 0x05;
    (int&)p[0] = (int)Float_Address((float)argument.num_); p+= sizeof(int);
    return;
  }
}
void AOT_Compiler::FPU_Store(std::string name)
{
  AOT_Var* var = Local_Direct(name);
  switch(var->type)
  {
  case _FLOAT:
    *p++ = 0xD9;
    *p++ = 0x5D;
    *p++ = two_complement_8(Local_Address(name) + 4);
    return;
  case _INT:
    *p++ = 0xDB;
    *p++ = 0x5D;
    *p++ = two_complement_8(Local_Address(name) + 4);
    return;
  case _DOUBLE:
    *p++ = 0xDD;
    *p++ = 0x5D;
    *p++ = two_complement_8(Local_Address(name) + 4);
    return;
  }
}
void AOT_Compiler::FPU_Add(FPU_REGISTERS reg)
{
  *p++ = 0xDE;
  *p++ = 0xC0 + (unsigned)reg;
}
void AOT_Compiler::FPU_Sub(FPU_REGISTERS reg)
{
  *p++ = 0xDE;
  *p++ = 0xE8 + (unsigned)reg;
}
void AOT_Compiler::FPU_Mul(FPU_REGISTERS reg)
{
  *p++ = 0xDE;
  *p++ = 0xC8 + (unsigned)reg;
}
void AOT_Compiler::FPU_Div(FPU_REGISTERS reg)
{
  *p++ = 0xDE;
  *p++ = 0xF8 + (unsigned)reg;
}
void AOT_Compiler::FPU_Sqr()
{
  *p++ = 0xDC;
  *p++ = 0xC8;
}
void AOT_Compiler::FPU_Abs()
{
  *p++ = 0xD9;
  *p++ = 0xE1;
}
void AOT_Compiler::FPU_Root()
{
  *p++ = 0xD9;
  *p++ = 0xFA;
}
void AOT_Compiler::FPU_One()
{
  *p++ = 0xD9;
  *p++ = 0xE8;
}
void AOT_Compiler::FPU_Zero()
{
  *p++ = 0xD9;
  *p++ = 0xEE;
}
void AOT_Compiler::FPU_PI()
{
  *p++ = 0xD9;
  *p++ = 0xEB;
}
void AOT_Compiler::FPU_Xchg()
{
  *p++ = 0xD9;
  *p++ = 0xC9;
}
void AOT_Compiler::FPU_Invert()
{
  *p++ = 0xD9;
  *p++ = 0xE0;
}
void AOT_Compiler::FPU_Neg()
{
  FPU_Abs();
  FPU_Invert();
}
void AOT_Compiler::FPU_Round()
{
  *p++ = 0xD9;
  *p++ = 0xFC;
}
void AOT_Compiler::FPU_Sin()
{
  *p++ = 0xD9;
  *p++ = 0xFE;
}
void AOT_Compiler::FPU_Cos()
{
  *p++ = 0xD9;
  *p++ = 0xFF;
}