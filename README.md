Dynamic Compiler
================

Sometimes the desired execution of a program can't be determined until information is available at runtime. Some solutions exist to this problem such as generating bytecode to be executed on a virtual machine. These solutions however are slower than executing native machine code. To keep runtime flexibility but achieve speed closer to native machine code JIT (Just in time) compilers exist to compile down key parts of the program. The rest of the program however still runs in a virtual machine.

The Dynamic Compiler instead uses an AOT (Ahead of time) compiler where the program is fully constructed before execution. This allows for the program to be completely independent of a virtual machine and run at full native speeds.

Abstraction
================
The compiler itself is wrapped in a C++ class allowing for easy manipulation and construction of programs. Code is written in a format similar to assembly with things such as labels and string pooling done manually by the class.

Supported Architectures
=================

Currently the only machine supported is x86 as it is the standard for most desktop and laptop computing. A version is in the works for both x64 and ARM. The top level interface however will remain constant allowing for easy switching between supported architecutres. The interface prefers x86 for optimizations but it is possible to map the generic structure to other architectures machine code.
