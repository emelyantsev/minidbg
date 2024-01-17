#include <iostream>

void print() {
    std::cerr << "Hello" << std::endl;
}

class Printer {

public:

    Printer() {

        printf("address of function print() is : %p\n", print); 
    }
};


Printer printer;

int main() {

    print();

    printf("address of function print() is : %p\n", print); 

    /* emulate segfault
    int* ptr = nullptr;

    *(ptr) += 1;
    */
}