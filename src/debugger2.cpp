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


void MiniDbg::Debugger::set_breakpoint_at_address( std::intptr_t addr ) {

    std::cout << "Setting breakpoint at address " << std::showbase << std::hex << addr << std::endl;

    Breakpoint bp( m_pid, addr );
    bp.Enable();

    m_breakpoints.emplace( addr, bp ) ;
}


uint64_t MiniDbg::Debugger::read_memory( uint64_t address ) {

    return ptrace( PTRACE_PEEKDATA, m_pid, address, nullptr );
}


void MiniDbg::Debugger::write_memory( uint64_t address, uint64_t value ) {

    ptrace( PTRACE_POKEDATA, m_pid, address, value );
}


uint64_t MiniDbg::Debugger::get_pc() {

    return get_register_value( m_pid, MiniDbg::Register::rip );
}


void MiniDbg::Debugger::set_pc( uint64_t pc )  {

    set_register_value( m_pid, MiniDbg::Register::rip, pc );
}


void MiniDbg::Debugger::step_over_breakpoint() {

    if ( m_breakpoints.count( get_pc() ) ) {

        MiniDbg::Breakpoint& bp = m_breakpoints.at( get_pc() );
        
        if ( bp.is_enabled() ) {
        
            bp.Disable();
            ptrace( PTRACE_SINGLESTEP, m_pid, nullptr, nullptr );
            wait_for_signal();
            bp.Enable();
        }
    }
}


void MiniDbg::Debugger::wait_for_signal() {
 
    int wait_status;
    int options = 0;

    waitpid( m_pid, &wait_status, options );
    process_status( wait_status );

    siginfo_t siginfo = get_signal_info();

    switch ( siginfo.si_signo ) {

        case SIGTRAP:
            handle_sigtrap( siginfo );
            break;
        case SIGSEGV:
            std::cout << "Yay, segfault. Reason: " << siginfo.si_code << std::endl;
            break;
        default:
            std::cout << "Got signal " << std::dec << siginfo.si_signo << " " << "\"" << strsignal( siginfo.si_signo ) << "\"" << std::endl;
    }
}


void MiniDbg::Debugger::dump_registers() {
 
    for ( const MiniDbg::RegDescriptor& rd : g_register_descriptors ) {

        std::cout << rd.name << " " << std::showbase << std::hex << get_register_value( m_pid, rd.r ) << std::endl;
    }
}


void MiniDbg::Debugger::initialise_load_address() {

    if ( m_elf.get_hdr().type == elf::et::dyn ) {
      
        std::ifstream map( "/proc/" + std::to_string( m_pid ) + "/maps" );
        std::string addr;
        std::getline( map, addr, '-' );
        m_load_address = std::stoul( addr, 0, 16 );
        
        std::cout << "Load address is " << std::showbase << std::hex << m_load_address << std::endl;
    }
}


uint64_t MiniDbg::Debugger::offset_load_address( uint64_t addr ) {

   return addr - m_load_address;
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

                throw std::out_of_range( "Cannot find line entry" );
            }
            else {

                return it;
            }
        }
    }

    std::cerr << "Cannot find line entry for address " << std::showbase << std::hex << pc << std::endl;
    throw std::out_of_range( "Cannot find line entry" );
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


siginfo_t MiniDbg::Debugger::get_signal_info() {

    siginfo_t info;
    ptrace( PTRACE_GETSIGINFO, m_pid, nullptr, &info );
    return info;
}


void MiniDbg::Debugger::handle_sigtrap( siginfo_t info ) {

    switch ( info.si_code ) {
        
        case SI_KERNEL:
        case TRAP_BRKPT:
        {
            set_pc( get_pc() - 1 );

            std::cout << "Hit breakpoint at address " << std::showbase << std::hex << get_pc() << std::endl;
            
            uint64_t offset_pc = offset_load_address( get_pc() ); 
            dwarf::line_table::iterator line_entry = get_line_entry_from_pc( offset_pc );
            
            print_source( line_entry->file->path, line_entry->line );     
            break;
        }
        case TRAP_TRACE:

            std::cout << "Got TRAP_TRACE code " << std::dec << info.si_code << std::endl;
            break;

        default:
            
            std::cout << "Unknown SIGTRAP code " << std::dec << info.si_code << std::endl;
            break;
    }
}


void MiniDbg::Debugger::single_step_instruction() {

    ptrace( PTRACE_SINGLESTEP, m_pid, nullptr, nullptr );
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


uint64_t MiniDbg::Debugger::offset_dwarf_address( uint64_t addr ) {

   return addr + m_load_address;
}


void MiniDbg::Debugger::set_breakpoint_at_function(const std::string& name) {

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