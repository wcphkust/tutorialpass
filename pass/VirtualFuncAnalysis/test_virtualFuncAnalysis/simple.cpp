#include <stdio.h>

class A {
public:
    virtual void foo() {printf("A");}
};
class B : public A {
public:
    virtual void foo() {printf("B");}
};
int main() {
    A *o1 = new B();
    o1->foo();
}
