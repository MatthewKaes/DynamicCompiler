Dynamic Compiler
================

Sometimes the desired execution of a program can't be determined until information is avalible at runtime. Some solutions exsist to this problem such as generating bytecode to be executed on a virtual machine. These solutions however are slower then executing native machine code. To keep runtime flexibility but achive speed closer to native machine code JIT (Just in time) compilers exsist to compile down key parts of the program. The rest of the program however still runs in a virtual machine.

The Dynamic Compiler instead uses an AOT (Ahead of time) compiler where the program is fully constructed before execution. This allows for the program to be completely independent of a virtual machine and run at full native speeds.
