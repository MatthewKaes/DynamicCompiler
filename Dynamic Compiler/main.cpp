#include "Compiler.h"

#pragma warning( disable : 4996)

int foo(int x)
{
  printf("Hello World! ");
  printf("External Function call.\n");
  return x * 4;
}

int main( void ) 
{

  int a;
  AOT_Compiler program("Test");
  program.Start_Function();
  program.Add_Variable("x");
  program.Add_Variable("y");
  program.Allocate_Stack();
  program.Load_Register(EAX, 10);
  program.Mul(EAX, EAX);
  program.Load_Local("x", EAX);
  program.Push(EAX);
  program.Call(foo);
  program.Push(EAX);
  program.Push("Print var: %d\n");
  program.Call(printf);
  program.Return("x");
  program.End_Function();
  
  a = program.Execute();
  printf("%d", a);
}