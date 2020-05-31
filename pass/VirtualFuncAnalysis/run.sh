export PATH=/path/to/llvm/bin:$PATH
export LLVM_HOME=/path/to/llvm/home
mkdir -p build && cd build && cmake .. && make && cd ..
echo "=============Simple.cpp============="
opt -load build/libVirtualCallAnalysisPass.so -virtual-call < target/simple.ll >> /dev/null
echo "=============Hard.cpp============="
opt -load build/libVirtualCallAnalysisPass.so -virtual-call < target/hard.ll >> /dev/null