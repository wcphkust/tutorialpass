#include <stdio.h>

class A {
public:
    virtual int foo() {printf("A"); return 1;}
    virtual int goo() {printf("AG"); return 1;}
};
class B : public A {
public:
    virtual int foo() {printf("B"); return 1;}
    virtual int goo() {printf("BG"); return 1;}
};
int main() {
    B *o2;
    A *o1 = new B();
    int u = o1->goo();
    return 0;
}
