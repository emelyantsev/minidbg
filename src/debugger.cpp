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
#include <linux/limits.h>

#include "debugger.hpp"
#include "registers.hpp"
#include "dwarf_helpers.hpp"
#include "linenoise.h"
#include "helpers.h"



MiniDbg::Debugger::Debugger( const std::string& prog_name ) : m_prog_name( prog_name ), m_state( State::NOT_RUNNING ) {}


int MiniDbg::Debugger::Run() {

    char* line = nullptr;

    while( ( line = linenoise( "(minidbg) " ) ) != nullptr ) {
        
        handle_command( line );
        linenoiseHistoryAdd( line );
        linenoiseFree( line );
    }

    return EXIT_SUCCESS;
}


void MiniDbg::Debugger::handle_command( const std::string& line ) {

    std::vector<std::string> args = split( line, ' ' );
    std::string command = args[0];

    if ( is_prefix( command, "run" ) ) {

        launch_debuggee( m_prog_name );
    }
    else if ( is_prefix( command, "attach") ) {

        attach_to_debuggee( std::stoi( args[1] ) ) ;
    }
    else if ( is_prefix( command, "detach") ) {

        detach_debuggee();
    }
    else if ( is_prefix( command, "cont" ) ) {

        continue_execution();
    }
    else if ( is_prefix( command, "break" ) ) {

        if ( args[1][0] == '0' && args[1][1] == 'x') {
            
            std::string addr (args[1], 2);
            set_breakpoint_at_address( std::stol(addr, 0, 16) );
        }
        else if ( args[1].find(':') != std::string::npos ) {

            std::vector<std::string> file_and_line = split( args[1], ':' ); 
            set_breakpoint_at_source_line( file_and_line[0], std::stoi( file_and_line[1]) );
        }
        else {

            set_breakpoint_at_function( args[1] );
        }
    }
    else if( is_prefix( command, "step" ) ) {
        
        step_in();
    }
    else if( is_prefix( command, "next" ) ) {
     
        step_over();
    }
    else if ( is_prefix( command, "finish" ) ) {
        
        step_out();
    }
    else if ( is_prefix( command, "register" ) ) {

        if ( is_prefix( args[1], "dump" ) ) {
            
            dump_registers();
        }
        else if ( is_prefix( args[1], "read" ) ) {
        
            std::cout << get_register_value( m_pid, get_register_from_name( args[2] ) ) << std::endl;
        }
        else if ( is_prefix( args[1], "write" ) ) {
        
            std::string val( args[3], 2 ); //assume 0xVAL
            set_register_value( m_pid, get_register_from_name(args[2]), std::stoul( val, 0, 16 ) );
        }
    }
    else if( is_prefix( command, "memory" ) ) {
        
        std::string addr ( args[2], 2 ); //assume 0xADDRESS

        if ( is_prefix( args[1], "read" ) ) {

            std::cout << std::hex << read_memory( std::stoul( addr, 0, 16 ) ) << std::endl;
        }
        else if ( is_prefix( args[1], "write" ) ) {
        
            std::string val (args[3], 2); //assume 0xVAL
            write_memory( std::stol( addr, 0, 16 ), std::stoul(val, 0, 16));
        }
    }
    else if( is_prefix( command, "stepi" ) ) {

        single_step_instruction_with_breakpoint_check();

        try {

            dwarf::line_table::iterator line_entry = get_line_entry_from_pc( offset_load_address( get_pc() ) );
            print_source( line_entry->file->path, line_entry->line );
        }
        catch ( std::exception& e ) {

            std::cerr << "Error printing source " << e.what() << std::endl;
        }
    }
    else if( is_prefix( command, "backtrace" ) ) {

        print_backtrace();
    }
    else if( is_prefix( command, "variables" ) ) {

        read_variables();
    }
    else if( is_prefix( command, "symbol" ) ) {

        std::vector<MiniDbg::Symbol> syms = lookup_symbol( args[1] );

        for (const MiniDbg::Symbol& s : syms) {

            std::cout << s.name << ' ' << to_string( s.type ) << " " << std::hex << s.addr << std::endl;
        }
    }
    else {
        std::cerr << "Unknown command " << command << std::endl;
    }
}


void MiniDbg::Debugger::continue_execution() {

    step_over_breakpoint();
    ptrace( PTRACE_CONT, m_pid, nullptr, nullptr );
    wait_for_signal();
}


void MiniDbg::Debugger::process_status( int status ) {

    if ( WIFEXITED( status ) ) {

        std::cout << "Debugee terminated normally with code " << std::dec << WEXITSTATUS( status ) << std::endl;
    }
    else if ( WIFSTOPPED( status ) ) {

        std::cout << "Debuggee was stopped by delivery of a signal " << std::dec << WSTOPSIG( status ) << std::endl;
    }
    else if ( WIFSIGNALED( status ) ) {

        std::cout << "Debugge terminated because of unhandled signal " << std::dec << WTERMSIG( status ) << std::endl;
    }
    else {

        std::cout << "Unknown waiting status" << std::endl;
    }
}





void MiniDbg::Debugger::execute_debuggee( const std::string& prog_name ) {

    if ( ptrace( PTRACE_TRACEME, 0, 0, 0 ) < 0 ) {
    
        std::cerr << "Error in ptrace\n";
        return;
    }
    
    execl( prog_name.c_str(), prog_name.c_str(), nullptr );

}

void MiniDbg::Debugger::attach_to_debuggee( const int pid ) {


    //clear_debuggee_data();

    m_pid = pid;
    m_prog_name = get_executable_path_by_pid( pid );

    m_fd = open( m_prog_name.c_str(), O_RDONLY );

    m_elf = elf::elf( elf::create_mmap_loader( m_fd ) );
    m_dwarf = dwarf::dwarf( dwarf::elf::create_loader( m_elf ) );

    if ( ::ptrace( PTRACE_ATTACH, pid, NULL, NULL ) < 0 ) {
    
        std::cerr << "Error in ptrace\n";
        clear_debuggee_data();
        return;
    }
    else {

        std::cout << "Attach to PID " << m_pid << " SUCCESS" << std::endl; 
        initialise_load_address();
        m_state = State::RUNNING;
    }
}

void MiniDbg::Debugger::launch_debuggee( const std::string& prog_name ) {


    m_fd = open( m_prog_name.c_str(), O_RDONLY );

    m_elf = elf::elf( elf::create_mmap_loader( m_fd ) );
    m_dwarf = dwarf::dwarf( dwarf::elf::create_loader( m_elf ) );

    pid_t pid = fork();

    if ( pid == 0 ) {  // child
        
        personality( ADDR_NO_RANDOMIZE );
        execute_debuggee( m_prog_name );
    }
    else {

        m_pid = pid;
    }

    wait_for_signal();
    initialise_load_address();
    m_state = State::RUNNING;
}

void MiniDbg::Debugger::clear_debuggee_data() {

    m_prog_name.clear();
    m_pid = 0;
    m_load_address = 0;
    m_state = State::NOT_RUNNING;
    m_breakpoints.clear();
    close( m_fd );
}

void MiniDbg::Debugger::detach_debuggee() {

    for (auto& [_, breakpoint] : m_breakpoints ) {

        if ( breakpoint.is_enabled() ) {
            breakpoint.Disable();
        }
    }

    if ( ::ptrace( PTRACE_DETACH, m_pid, NULL, NULL ) < 0 ) {
    
        std::cerr << "Error in ptrace\n";
        return;
    }

    clear_debuggee_data();
    m_state = State::NOT_RUNNING;
}


std::string MiniDbg::Debugger::get_executable_path_by_pid( const int pid ) {

    std::string path_name = "/proc/" + std::to_string( pid ) + "/exe" ;
    char buf[PATH_MAX];

    int result = readlink( path_name.c_str(), buf, PATH_MAX ) ;
    assert( result >= 0 ) ;

    return std::string( buf );
}