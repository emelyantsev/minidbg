#include <iostream>

#include "debugger.hpp"

int main( int argc, char* argv[] ) {

    if ( argc < 2 )  {
    
        std::cerr << "Usage: minidbg <program_name>" << std::endl;
        return -1;
    }

    char* prog = argv[1];
    
    MiniDbg::Debugger debugger( prog );

    return debugger.Run();
}