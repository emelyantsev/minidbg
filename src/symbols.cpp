#include "symbols.hpp"


namespace MiniDbg {


std::string to_string( SymbolType st ) {

    switch ( st ) {

        case SymbolType::notype: return "notype";
        case SymbolType::object: return "object";
        case SymbolType::func: return "func";
        case SymbolType::section: return "section";
        case SymbolType::file: return "file";

        default: return "";
    }
}

SymbolType to_symbol_type( elf::stt sym ) {

    switch ( sym ) {

        case elf::stt::notype: return SymbolType::notype;
        case elf::stt::object: return SymbolType::object;
        case elf::stt::func: return SymbolType::func;
        case elf::stt::section: return SymbolType::section;
        case elf::stt::file: return SymbolType::file;
        
        default: return SymbolType::notype;
    }
}

} // end namespace MiniDbg