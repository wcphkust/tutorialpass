# Clang Static Analyzer Type Analysis Plugin

Make sure you have installed or configured paths for LLVM >=9.0. For macOS user, you can run `brew install llvm clang` directly.

To build with cmake:
```
mkdir build
cd build
cmake ..
```

To build with meson and ninja:

```.sh
mkdir build
cd build
meson ..
ninja
```

To run with `clang`:
```
clang -Xclang -Xclang -load -Xclang ./libClangTypeAnalysis.so -Xclang -analyze -Xclang -analyzer-checker=homeowrk.ClangTypeAnalysis -g -c ../../../test/test_TypeAnalysis/lambda.cpp
```

For macOS user:
```
clang-10 -Xclang -load -Xclang cmake-build-release/libClangTypeAnalysis.dylib -Xclang -analyze -Xclang -analyzer-checker=homework.ClangTypeAnalysis -g -c test/lambda.cpp -v
```
