# Get Started for LLVM Pass

## Basic Info

This is a pass collection of static analyzers based on LLVM, which implement the instance introduced in each chapter of [Anders Møller's note](https://cs.au.dk/~amoeller/spa/).

---

## Pass List

The typical passes are as follows:

- `ParameterCounter`: Count the (floating-point) arguments of each function.
- `LiveVariableViaInst`: Perform an intraprocedural path-insensitive live variable analysis for a C program. Transfer liveness information in the granularity of instructions.
- `LiveVariableViaBB`: Perform an intraprocedural path-insensitive live variable analysis for a C program. Transfer liveness information in the granularity of basic blocks.
- `FileStateSimulator`: Check file property by intraprocedural path-sensitive analysis based on collecting paths exhaustively.
- `InterSignAnalysis`: Analyze the sign information of integral variables by function clone based interprocedural analysis.
- `VirtualFuncAnalysis`: Analyze the virtual calls based on CHA(Class Hierarchy Analysis) and RTA(Rapid Type Analysis).
- `IntervalAnalysis`: Perform an intraprocedural range analysis based on abstract interpretation on interval domain.

---

## Useful Link

- [My LLVM note](https://www.notion.so/LLVM-15c86e75470645f99b0f7d950603a683)
- [Static Analysis Course Web](https://www.notion.so/Static-Analysis-Course-ba01aa357d6d4fbca9b31ac3913a9a3e)