#include <iostream>
#include <limits.h>


template<typename T>
void print_type(T t) {

    std::cout << sizeof(T) << std::endl;
    std::cout << typeid(T).name() << std::endl;
    std::cout << t << std::endl << std::endl;
}


int main() {

    long x = 0xFF;

    print_type(x);

    unsigned long long x2 = 0xFFFFFFFFFFFFFFFF;

    print_type(x2) ;


    long long x3 = 0xFFFFFFFFFFFFFFFF;

    print_type(x3) ;

    long long z1 = -1;
    unsigned long long z2 = z1;

    print_type(z1);
    print_type(z2);

    unsigned long long y1 = ULLONG_MAX;

    long long y2 = y1;

    print_type(y1);
    print_type(y2);


    unsigned long long l1 = 0xffffffffffffffff;

    uint8_t t1 = l1;

    print_type(l1);
    print_type(t1);





    return 0;
}