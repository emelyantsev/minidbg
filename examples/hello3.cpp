#include <iostream>

void print(uint64_t val) {

    std::cout << val << std::endl;
}


int main() {

    uint64_t x = 845;

    ::printf("address of variable x is : %p\n", &x);

    print(x);

    ++x;

    print(x);
    
    return 0;
}