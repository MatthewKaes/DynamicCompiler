#include "Compiler.h"

#pragma warning( disable : 4996)

char system_colors[2];

int arg1;
int arg2;
int res1;

int foo()
{
  float x = 5.8f;
  float y = 25.3f;
  int z;
  scanf("", &x, &y, &z);
  z = x + y;
 __asm{
   fldpi
   fcos
   fstp x
 }
  return 0;
}
float foo4( double x, float y)
{
  printf("Hello World! ");
  printf("External Function call.\n");
  return x * 4;
}

class test_obj
{
public:
  int x;
  int y;
};
void footest(test_obj test)
{
  printf("%d", test.x);
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
  program.Call(foo4);
  program.Push(EAX);
  program.Push("Print var: %d\n");
  program.Call(printf);
  program.Return("x");
  program.End_Function();
  
  a = program.Execute();
  printf("%d", a);
}