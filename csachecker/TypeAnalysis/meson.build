project('ClangTypeAnalysis', 'cpp', default_options : ['c_std=c11', 'cpp_std=c++11'])

cxx = meson.get_compiler('cpp')

# from https://github.com/mesonbuild/meson/issues/1899
dep_llvm = dependency('llvm', version: '>= 6.0.0')
llvm_libdir = dep_llvm.get_configtool_variable('libdir')
dep_libs = [
    cxx.find_library('clangBasic', dirs : llvm_libdir),
    cxx.find_library('clangEdit', dirs : llvm_libdir),
    cxx.find_library('clangFrontend', dirs : llvm_libdir),
    cxx.find_library('clangStaticAnalyzerFrontend', dirs : llvm_libdir),
    cxx.find_library('clangStaticAnalyzerCore', dirs : llvm_libdir),
    cxx.find_library('clangStaticAnalyzerCheckers', dirs : llvm_libdir),
    cxx.find_library('clangSerialization', dirs : llvm_libdir),
    cxx.find_library('clangSema', dirs : llvm_libdir),
    cxx.find_library('clangAnalysis', dirs : llvm_libdir),
    cxx.find_library('clangCrossTU', dirs : llvm_libdir),
    cxx.find_library('clangRewriteFrontend', dirs : llvm_libdir),
    cxx.find_library('clangRewrite', dirs : llvm_libdir),
    cxx.find_library('clangLex', dirs : llvm_libdir),
    cxx.find_library('clangAST', dirs : llvm_libdir),
    cxx.find_library('clangASTMatchers', dirs : llvm_libdir),
    cxx.find_library('clangDynamicASTMatchers', dirs : llvm_libdir),
    cxx.find_library('clangIndex', dirs : llvm_libdir),
    cxx.find_library('clangDriver', dirs : llvm_libdir),
    cxx.find_library('clangParse', dirs : llvm_libdir),
    cxx.find_library('LLVM', dirs : llvm_libdir)
]

library('ClangTypeAnalysis', 'ClangTypeAnalysis.cpp', dependencies: dep_libs)