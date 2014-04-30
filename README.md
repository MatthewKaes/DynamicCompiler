Dynamic Compiler
================

Sometimes the desired execution of a program can't be determined until information is available at runtime. Some solutions exist to this problem such as generating bytecode to be executed on a virtual machine. These solutions however are slower than executing native machine code. To keep runtime flexibility but achieve speed closer to native machine code JIT (Just in time) compilers exist to compile down key parts of the program. The rest of the program however still runs in a virtual machine.

The Dynamic Compiler instead uses an AOT (Ahead of time) compiler where the program is fully constructed before execution. This allows for the program to be completely independent of a virtual machine and run at full native speeds.

Abstraction
================
The compiler itself is wrapped in a C++ class allowing for easy manipulation and construction of programs.
