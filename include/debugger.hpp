#ifndef MINIDBG_DEBUGGER_HPP
#define MINIDBG_DEBUGGER_HPP

#include <utility>
#include <string>
#include <unordered_map>
#include <linux/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "breakpoint.hpp"

#include "dwarf/dwarf++.hh"
#include "elf/elf++.hh"

#include "symbols.hpp"

namespace MiniDbg {

    class Debugger {
    
    public:

        Debugger( const std::string& prog_name, pid_t pid ) ;

        void Run();

    private:
    
        void handle_command( const std::string& line );

        void continue_execution();
        void process_status( int status );

        void set_breakpoint_at_address( std::intptr_t addr );   
        void set_breakpoint_at_function( const std::string& name ); 
        void set_breakpoint_at_source_line( const std::string& file, unsigned line );

        void dump_registers(); 
        void print_backtrace();
        
        uint64_t get_pc();
        uint64_t get_offset_pc();
        void set_pc( uint64_t pc );

        void step_over_breakpoint();
        void wait_for_signal();
        siginfo_t get_signal_info();
        void handle_sigtrap( siginfo_t info );

        void initialise_load_address();
        uint64_t offset_load_address( uint64_t addr );
        uint64_t offset_dwarf_address( uint64_t addr );

        dwarf::die get_function_from_pc( uint64_t pc );
        dwarf::line_table::iterator get_line_entry_from_pc( uint64_t pc );

        uint64_t read_memory( uint64_t address );
        void write_memory( uint64_t address, uint64_t value );   
        
        void print_source( const std::string& file_name, unsigned line, unsigned n_lines_context = 2 );

        void single_step_instruction();
        void single_step_instruction_with_breakpoint_check();

        void step_out();
        void step_in();
        void step_over();

        void remove_breakpoint(std::intptr_t addr);

        std::vector<Symbol> lookup_symbol(const std::string& name);

    private:    

        std::string m_prog_name;
        pid_t m_pid;
        std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
        uint64_t m_load_address = 0;
        dwarf::dwarf m_dwarf;
        elf::elf m_elf;
    };
}

#endif
