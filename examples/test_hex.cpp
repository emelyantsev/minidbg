#include <iostream>
#include <vector>
#include <bitset>
#include <sstream>


enum class Mode {
    BIN,
    OCT,
    DEC,
    HEX
};


void print(int x, const Mode mode) {

    switch ( mode ) {

        case Mode::BIN: 
            std::cout << std::bitset< sizeof(int) * 8 >(x) << std::endl; 
            break;
        case Mode::OCT:
            std::cout << std::showbase << std::oct << x << std::endl;
            break;
        case Mode::DEC:
            std::cout << std::showbase << std::dec << x << std::endl;
            break;
        case Mode::HEX:
            std::cout << std::showbase << std::hex << x << std::endl;
            break;
        default:
            break;   
    }
}

int main() {

    int x = 2147483647;

    std::cout << std::dec << x << std::endl;
    std::cout << std::oct << x << std::endl;
    std::cout << std::hex << x << std::endl;

    std::cout << 42 << std::endl;

    std::vector<Mode> modes = { Mode::BIN, Mode::OCT, Mode::DEC, Mode::HEX } ;

    for (const auto& mode : modes) {

        print(x, mode);
    }

    // ---------------------------


    int n;
    std::istringstream("0x2A") >> std::hex >> n;

    std::cout << std::dec << "Parsing \"0x2A\" as hex gives " << n << '\n';


    std::istringstream("52") >> std::oct >> n;

    std::cout << std::dec << "Parsing \"52\" as oct gives " << n << '\n';


    std::istringstream("0x12") >> std::hex >> n;

    std::cout << std::dec << "Parsing \"0x12\" as hex gives " << n << '\n';

    // -------------------------

    
    std::vector<int> bases = {8, 10, 16} ;

    for (int base : bases) {

        std::cout << std::stoi("42", 0, base) << std::endl;
    }


    std::cout << std::stoi("0x2A", 0, 16) << std::endl;

    std::cout << std::stoi("0x7fffffff", 0, 16) << std::endl;

    try {

        std::cout << std::stoi("0x80000000", 0, 16) << std::endl;
    }
    catch (const std::exception& e) {

        std::cout << "Caught exception " << typeid(e).name() << " " << "\"" << e.what() << "\"" << std::endl;
    }

    
    return 0;
}