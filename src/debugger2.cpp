#include <vector>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/personality.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>

#include "debugger.hpp"
#include "registers.hpp"
#include "dwarf_helpers.hpp"
#include "linenoise.h"
#include "helpers.h"



uint64_t MiniDbg::Debugger::get_pc() {

    return get_register_value( m_pid, MiniDbg::Register::rip );
}


void MiniDbg::Debugger::set_pc( uint64_t pc )  {

    set_register_value( m_pid, MiniDbg::Register::rip, pc );
}


void MiniDbg::Debugger::initialize_load_address() {

    if ( m_elf.get_hdr().type == elf::et::dyn ) {
      
        std::ifstream map( "/proc/" + std::to_string( m_pid ) + "/maps" );
        std::string addr;
        std::getline( map, addr, '-' );
        m_load_address = std::stoul( addr, 0, 16 );
        std::cout << "Load address is 0x" << std::hex << m_load_address << std::endl;
    }
}


uint64_t MiniDbg::Debugger::offset_load_address( uint64_t addr ) {

   return addr - m_load_address;
}

uint64_t MiniDbg::Debugger::offset_dwarf_address( uint64_t addr ) {

   return addr + m_load_address;
}


std::string MiniDbg::Debugger::get_executable_path_by_pid( const int pid ) {

    std::string path_name = "/proc/" + std::to_string( pid ) + "/exe" ;
    char buf[PATH_MAX];

    int result = readlink( path_name.c_str(), buf, PATH_MAX ) ;
    assert( result >= 0 ) ;

    return std::string( buf );
}




void MiniDbg::Debugger::set_breakpoint_at_address( std::intptr_t addr ) {

    std::cout << "Setting breakpoint at address 0x" << std::hex << addr << std::endl;

    Breakpoint bp( m_pid, addr );
    bp.Enable();

    m_breakpoints.emplace( addr, bp ) ;
}


void MiniDbg::Debugger::step_over_breakpoint() {

    if ( m_breakpoints.count( get_pc() ) ) {

        MiniDbg::Breakpoint& bp = m_breakpoints.at( get_pc() );
        
        if ( bp.is_enabled() ) {
        
            bp.Disable();
            ::ptrace( PTRACE_SINGLESTEP, m_pid, nullptr, nullptr );
            wait_for_signal();
            bp.Enable();
        }
    }
}









dwarf::die MiniDbg::Debugger::get_function_from_pc( uint64_t pc ) {

    for  ( const dwarf::compilation_unit& cu : m_dwarf.compilation_units() ) {
    
        if ( dwarf::die_pc_range( cu.root() ).contains( pc ) ) {
    
            for ( const dwarf::die& die : cu.root() ) {
    
                if ( die.tag == dwarf::DW_TAG::subprogram ) {
    
                    if ( dwarf::die_pc_range( die ).contains( pc ) ) {

                        return die;
                    }
                }
            }
        }
    }

    throw std::out_of_range( "Cannot find function" );
}


dwarf::line_table::iterator MiniDbg::Debugger::get_line_entry_from_pc( uint64_t pc ) {

    for ( const dwarf::compilation_unit& cu : m_dwarf.compilation_units() ) {

        if ( dwarf::die_pc_range( cu.root() ).contains( pc ) ) {
        
            const dwarf::line_table& lt = cu.get_line_table();
            dwarf::line_table::iterator it = lt.find_address( pc );
        
            if ( it == lt.end() ) {

                std::cerr << "[" << "Can't find line entry for address 0x" << std::hex << pc << "]" << std::endl;
                throw std::out_of_range( "Can't find line entry" );
            }
            else {
                
                return it;
            }
        }
    }

    std::cerr << "[" << "Can't find compilation unit for address 0x" << std::hex << pc << "]" << std::endl;
    throw std::out_of_range( "Can't find compilation unit" );
}


void MiniDbg::Debugger::print_source( const std::string& file_name, unsigned line, unsigned n_lines_context ) {

    std::ifstream file ( file_name );

    unsigned int start_line = line <= n_lines_context ? 1 : line - n_lines_context;
    unsigned int end_line = line + n_lines_context + ( line < n_lines_context ? n_lines_context - line : 0 ) + 1;

    char c;
    unsigned int current_line = 1u;
    
    while ( current_line != start_line && file.get( c ) ) {
    
        if (c == '\n') {
            ++current_line;
        }
    }

    std::cout << ( current_line == line ? "> " : "  " );

    while ( current_line <= end_line && file.get( c ) ) {

        std::cout << c;

        if ( c == '\n' ) {

            ++current_line;
            std::cout << ( current_line == line ? "> " : "  " );
        }
    }

    std::cout << std::endl;
}




void MiniDbg::Debugger::single_step_instruction() {

    ::ptrace( PTRACE_SINGLESTEP, m_pid, nullptr, nullptr );
    wait_for_signal();
}


void MiniDbg::Debugger::single_step_instruction_with_breakpoint_check() {
    
    if ( m_breakpoints.count( get_pc() ) ) {
    
        step_over_breakpoint();
    }
    else {

        single_step_instruction();
    }
}


void MiniDbg::Debugger::remove_breakpoint( std::intptr_t addr ) {

    if ( m_breakpoints.at( addr ).is_enabled() ) {
        
        m_breakpoints.at( addr ).Disable();
    }

    m_breakpoints.erase( addr );
}


void MiniDbg::Debugger::step_out() {

    uint64_t frame_pointer = get_register_value( m_pid, Register::rbp );
    uint64_t return_address = read_memory( frame_pointer + 8 );

    bool should_remove_breakpoint = false;

    if ( !m_breakpoints.count( return_address ) ) {
        
        set_breakpoint_at_address( return_address );
        should_remove_breakpoint = true;
    }

    continue_execution();

    if ( should_remove_breakpoint ) {

        remove_breakpoint( return_address );
    }
}


uint64_t MiniDbg::Debugger::get_offset_pc() {

    return offset_load_address( get_pc() );
}

void MiniDbg::Debugger::step_in() {

   unsigned int line = get_line_entry_from_pc( get_offset_pc() )->line;

   while ( get_line_entry_from_pc( get_offset_pc() )->line == line ) {
      
      single_step_instruction_with_breakpoint_check();
   }

   dwarf::line_table::iterator line_entry = get_line_entry_from_pc( get_offset_pc() );

   print_source( line_entry->file->path, line_entry->line );
}


void MiniDbg::Debugger::step_over() {

    dwarf::die func = get_function_from_pc( get_offset_pc() );
    dwarf::taddr func_entry = dwarf::at_low_pc( func );
    dwarf::taddr func_end = dwarf::at_high_pc( func );

    dwarf::line_table::iterator line = get_line_entry_from_pc( func_entry ) ;
    dwarf::line_table::iterator start_line = get_line_entry_from_pc( get_offset_pc() );

    std::vector<std::intptr_t> to_delete;

    while ( line->address < func_end ) {

        uint64_t load_address = offset_dwarf_address( line->address );

        if ( line->address != start_line->address && !m_breakpoints.count( load_address ) ) {
            
            set_breakpoint_at_address( load_address );
            to_delete.push_back( load_address );
        }

        ++line;
    }

    uint64_t frame_pointer = get_register_value( m_pid, Register::rbp );
    uint64_t return_address = read_memory( frame_pointer + 8 );

    if ( !m_breakpoints.count( return_address ) ) {

        set_breakpoint_at_address( return_address );
        to_delete.push_back( return_address );
    }

    continue_execution();

    for ( std::intptr_t addr : to_delete ) {
        
        remove_breakpoint( addr );
    }
}

void MiniDbg::Debugger::set_breakpoint_at_function( const std::string& name ) {

    for ( const dwarf::compilation_unit& cu : m_dwarf.compilation_units() ) {
        
        for ( const dwarf::die& die : cu.root() ) {

            if ( die.has( dwarf::DW_AT::name ) && dwarf::at_name( die ) == name ) {

                dwarf::taddr low_pc = dwarf::at_low_pc( die );
                dwarf::line_table::iterator entry = get_line_entry_from_pc( low_pc );
                ++entry; //skip prologue
                set_breakpoint_at_address( offset_dwarf_address( entry->address ) );
            }
        }
    }

    std::cerr << "[" << "Can't find address of function " << name << "]" << std::endl;
}


void MiniDbg::Debugger::set_breakpoint_at_source_line( const std::string& file, unsigned line ) {

    for ( const dwarf::compilation_unit& cu : m_dwarf.compilation_units() )  {

        if ( is_suffix( file, dwarf::at_name( cu.root() ) ) ) {
            
            const dwarf::line_table& lt = cu.get_line_table();

            for ( const dwarf::line_table::entry& entry : lt ) {
            
                if ( entry.is_stmt && entry.line == line ) {
                    
                    set_breakpoint_at_address( offset_dwarf_address( entry.address ) );
                    return;
                }
            }
        }
    }

    std::cerr << "[" << "Can't find address for file \"" << file << "\" line \"" << line << "\"]" << std::endl;
}


