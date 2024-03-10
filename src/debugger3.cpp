#include <vector>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "debugger.hpp"
#include "registers.hpp"
#include "dwarf_helpers.hpp"
#include "linenoise.h"
#include "helpers.h"


uint64_t MiniDbg::Debugger::read_memory( uint64_t address ) {

    return ::ptrace( PTRACE_PEEKDATA, m_pid, address, nullptr );
}


void MiniDbg::Debugger::write_memory( uint64_t address, uint64_t value ) {

    ::ptrace( PTRACE_POKEDATA, m_pid, address, value );
}


void MiniDbg::Debugger::dump_registers() {
 
    for ( const MiniDbg::RegDescriptor& rd : g_register_descriptors ) {

        std::cout << rd.name << " " << std::showbase << std::hex << get_register_value( m_pid, rd.r ) << std::endl;
    }
}


std::vector<MiniDbg::Symbol> MiniDbg::Debugger::lookup_symbol( const std::string& name ) {

   std::vector<MiniDbg::Symbol> syms;

   for ( const elf::section& sec : m_elf.sections() ) {

        if ( sec.get_hdr().type != elf::sht::symtab && sec.get_hdr().type != elf::sht::dynsym ) {
            
            continue;
        }

        for ( const elf::sym& sym : sec.as_symtab() ) {
            
            if ( sym.get_name() == name ) {

                const elf::Sym<>& d = sym.get_data();
                syms.push_back( MiniDbg::Symbol{ MiniDbg::to_symbol_type( d.type() ), sym.get_name(), d.value } );
            }
        }
   }

   return syms;
}


void MiniDbg::Debugger::print_backtrace() {

    auto output_frame = [frame_number = 0] (auto&& func) mutable {

        std::cout << "frame #" << frame_number++ << ": " << std::hex << dwarf::at_low_pc( func )
                  << ' ' << dwarf::at_name( func ) << std::endl;
    };

    dwarf::die current_func = get_function_from_pc( offset_load_address( get_pc() ) );
    output_frame( current_func );

    uint64_t frame_pointer = get_register_value( m_pid, Register::rbp ) ;
    uint64_t return_address = read_memory( frame_pointer + 8 ) ;

    while ( dwarf::at_name( current_func ) != "main") {

        current_func = get_function_from_pc( offset_load_address( return_address ) );
        output_frame( current_func );
        frame_pointer = read_memory( frame_pointer );
        return_address = read_memory( frame_pointer + 8 );
    }
}


void MiniDbg::Debugger::read_variables() {

    dwarf::die func = get_function_from_pc( get_offset_pc() );

    for ( const dwarf::die& die : func ) {

        if ( die.tag == dwarf::DW_TAG::variable ) {
            
            dwarf::value loc_val = die[ dwarf::DW_AT::location ];

            if ( loc_val.get_type() == dwarf::value::type::exprloc ) {   //only supports exprlocs for now

                ptrace_expr_context context( m_pid, m_load_address );
                dwarf::expr_result result = loc_val.as_exprloc().evaluate( &context );

                switch ( result.location_type ) {

                    case dwarf::expr_result::type::address:
                    {
                        dwarf::taddr offset_addr = result.value;
                        uint64_t value = read_memory( offset_addr );
                        std::cout << dwarf::at_name( die ) << " (" << std::hex << offset_addr << ") = " << value << std::endl;
                        break;
                    }

                    case dwarf::expr_result::type::reg:
                    {
                        uint64_t value = get_register_value_from_dwarf_register( m_pid, result.value );
                        std::cout << dwarf::at_name( die ) << " (reg " << std::hex << result.value << ") = " << value << std::endl;
                        break;
                    }

                    default:
                        throw std::runtime_error( "Unhandled variable location" );
                }
            }
            else {
                
                throw std::runtime_error( "Unhandled variable location" );
            }
        }
    }
}