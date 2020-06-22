//#include "util/demangle.h"
//
//using namespace std;
//
//// demangle c++ name: _ZN1BC1Ev -> B::B()
//static std::string cxx_demangle(const std::string &mangled_name) {
//
//    int status = 0;
//    char *demangled = abi::__cxa_demangle(mangled_name.c_str(), NULL, NULL, &status);
//    std::string result((status == 0 && demangled != NULL) ? demangled
//                                                          : mangled_name);
//    free(demangled);
//    return result;
//}
//
//
//// get class name from function name: B::B() -> B
//static std::string demangle_class_name(const std::string &demangled_name) {
//
//    int found = demangled_name.find("::");
//    return demangled_name.substr(0, found);
//}
//
//// get class name used by LLVM: A -> class.A
//static std::string llvm_class_name(const std::string &classN) {
//
//    return "class." + classN;
//}
//
//// get only function name : A::foo(int) -> foo(int)
//static std::string get_called_function_name(const std::string s) {
//
//    int found = s.find("::");
//    return s.substr(found + 2);
//}