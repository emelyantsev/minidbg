#ifndef MINIDBG_SYMBOLS_HPP
#define MINIDBG_SYMBOLS_HPP

#include <string>

#include "elf/elf++.hh"

namespace MiniDbg {

    enum class SymbolType {

        notype,            // No type (e.g., absolute symbol)
        object,            // Data object
        func,              // Function entry point
        section,           // Symbol is associated with a section
        file,              // Source file associated with the
    };                     // object file

    std::string to_string( SymbolType st ) ;

    SymbolType to_symbol_type( elf::stt sym ) ;

    struct Symbol {

        SymbolType type;
        std::string name;
        std::uintptr_t addr;
    };

}

#endif