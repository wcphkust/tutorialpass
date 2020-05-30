#include <stdio.h>

class A {
public:
    virtual void foo(int n) {printf("A");}
};
class B : public A {
public:
    virtual void foo(int n) {printf("B");}
};
class C : public A {
public:
    virtual void foo(int n) {printf("C");}
};
class D : public C {
public:
    virtual void foo(int n) {printf("D");}
};

void _not_analyze() {

    // retain class info for D
    D *d = new D();
}

int main() {
    B *o1 = new B();
    o1->foo(2);
    C *o2 = new C();
    o2->foo(4);
    A *o3;
    o3 = o2;
    o3->foo(6);
}
