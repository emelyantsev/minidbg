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

#include "linenoise.h"


void execute_debugee ( const std::string& prog_name ) ;


/* -----------------  Main ------------------- */ 


int main( int argc, char* argv[] ) {

    if ( argc < 2 )  {
    
        std::cerr << "Program name not specified";
        return -1;
    }

    char* prog = argv[1];
    pid_t pid = fork();

    if ( pid == 0 ) {  // child
        
        personality( ADDR_NO_RANDOMIZE );
        execute_debugee( prog );
    }
    else if ( pid >= 1 )  {   // parent
        
        std::cout << "Started debugging process " << pid << '\n';
        MiniDbg::Debugger dbg( prog, pid );
        dbg.Run();
    }

    return EXIT_SUCCESS;
}


/* ----------------- end main -------------------- */


/* ------------------- helpers ------------------- */

void execute_debugee ( const std::string& prog_name ) {
    
    if ( ptrace( PTRACE_TRACEME, 0, 0, 0 ) < 0) {
    
        std::cerr << "Error in ptrace\n";
        return;
    }
    
    execl( prog_name.c_str(), prog_name.c_str(), nullptr );
}


std::vector<std::string> split( const std::string& s, char delimiter ) {

    std::vector<std::string> out;
    std::stringstream ss( s );
    std::string item;

    while ( std::getline( ss, item, delimiter ) ) 
    {
        out.push_back(item);
    }

    return out;
}


bool is_prefix( const std::string& s, const std::string& of ) {
    
    if ( s.size() > of.size() ) {
        
        return false;
    }
        
    return std::equal( s.begin(), s.end(), of.begin() );
}


/* ----------------- Debugger -------------- */


MiniDbg::Debugger::Debugger( const std::string& prog_name, pid_t pid ) : m_prog_name( prog_name ), m_pid( pid ) {

    int fd = open( m_prog_name.c_str(), O_RDONLY );

    m_elf = elf::elf( elf::create_mmap_loader( fd ) );
    m_dwarf = dwarf::dwarf( dwarf::elf::create_loader( m_elf ) );
}

void MiniDbg::Debugger::Run() {

    wait_for_signal();
    initialise_load_address();

    char* line = nullptr;

    while( ( line = linenoise( "minidbg> " ) ) != nullptr ) {
        
        handle_command( line );
        linenoiseHistoryAdd( line );
        linenoiseFree( line );
    }
}

void MiniDbg::Debugger::handle_command(const std::string& line) {

    std::vector<std::string> args = split( line, ' ' );
    std::string command = args[0];

    if ( is_prefix( command, "cont" ) ) {

        continue_execution();
    }
    else if(is_prefix(command, "break")) {
    
        std::string addr { args[1], 2 };
        set_breakpoint_at_address( std::stoll(addr, 0, 16) );
    }
    else if (is_prefix(command, "register")) {

        if (is_prefix(args[1], "dump")) {
            dump_registers();
        }
        else if (is_prefix(args[1], "read")) {
        
            std::cout << get_register_value( m_pid, get_register_from_name(args[2]) ) << std::endl;
        }
        else if (is_prefix(args[1], "write")) {
        
            std::string val {args[3], 2}; //assume 0xVAL
            set_register_value( m_pid, get_register_from_name(args[2]), std::stoll(val, 0, 16) );
        }
    }
    else if(is_prefix(command, "memory")) {
        
        std::string addr {args[2], 2}; //assume 0xADDRESS

        if (is_prefix(args[1], "read")) {
            std::cout << std::hex << read_memory(std::stol(addr, 0, 16)) << std::endl;
        }
        if (is_prefix(args[1], "write")) {
        
            std::string val {args[3], 2}; //assume 0xVAL
            write_memory(std::stol(addr, 0, 16), std::stol(val, 0, 16));
        }
    }
    else {
        std::cerr << "Unknown command\n";
    }
}


void MiniDbg::Debugger::continue_execution() {

    step_over_breakpoint();
    ptrace(PTRACE_CONT, m_pid, nullptr, nullptr);
    wait_for_signal();
}

void MiniDbg::Debugger::process_status(int status) {

    if ( WIFEXITED( status ) ) {

        std::cout << "Debugee terminated normally with code " << WEXITSTATUS( status ) << std::endl;
    }
    else if ( WIFSTOPPED( status ) ) {

        std::cout << "Debuggee was stopped by delivery of a signal " << WSTOPSIG( status ) << std::endl;
    }
    else {

        std::cout << "Unknown waiting status" << std::endl;
    }
}

void MiniDbg::Debugger::set_breakpoint_at_address( std::intptr_t addr ) {

    std::cout << "Set breakpoint at address " << std::showbase << std::hex << addr << std::endl;

    Breakpoint bp(m_pid, addr);
    bp.Enable();

    m_breakpoints.emplace( addr, bp ) ;

}

uint64_t MiniDbg::Debugger::read_memory(uint64_t address) {

    return ptrace(PTRACE_PEEKDATA, m_pid, address, nullptr);
}

void MiniDbg::Debugger::write_memory(uint64_t address, uint64_t value) {
    ptrace(PTRACE_POKEDATA, m_pid, address, value);
}

uint64_t MiniDbg::Debugger::get_pc() {
    return get_register_value(m_pid, MiniDbg::reg::rip);
}

void MiniDbg::Debugger::set_pc(uint64_t pc) {
    set_register_value(m_pid, MiniDbg::reg::rip, pc);
}

void MiniDbg::Debugger::step_over_breakpoint() {

    if (m_breakpoints.count( get_pc() ) ) {

        auto& bp = m_breakpoints.at( get_pc() );
        
        if (bp.is_enabled()) {
        
            bp.Disable();
            ptrace(PTRACE_SINGLESTEP, m_pid, nullptr, nullptr);
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
            std::cout << "Got signal " << strsignal( siginfo.si_signo ) << std::endl;
    }
}

void MiniDbg::Debugger::dump_registers() {
 
    for (const auto& rd : g_register_descriptors) {

        std::cout << rd.name << " 0x" << std::setfill('0') << std::setw(16) << std::hex << get_register_value(m_pid, rd.r) << std::endl;
    }
}


void MiniDbg::Debugger::initialise_load_address() {

    if ( m_elf.get_hdr().type == elf::et::dyn ) {
      
        std::ifstream map( "/proc/" + std::to_string(m_pid) + "/maps" );
        std::string addr;
        std::getline( map, addr, '-' );
        m_load_address = std::stoul( addr, 0, 16 );

        std::cout << "Load address is " << std::showbase << std::hex << m_load_address << std::endl;
    }
}

uint64_t MiniDbg::Debugger::offset_load_address(uint64_t addr) {

   return addr - m_load_address;
}

dwarf::die MiniDbg::Debugger::get_function_from_pc(uint64_t pc) {

    for (auto &cu : m_dwarf.compilation_units() ) {
    
        if (die_pc_range( cu.root() ).contains(pc) ) {
    
            for (const auto& die : cu.root()) {
    
                if (die.tag == dwarf::DW_TAG::subprogram) {
    
                    if (die_pc_range(die).contains(pc)) {
                        return die;
                    }
                }
            }
        }
    }

    throw std::out_of_range{"Cannot find function"};
}

dwarf::line_table::iterator MiniDbg::Debugger::get_line_entry_from_pc(uint64_t pc) {

    for ( auto &cu : m_dwarf.compilation_units() ) {

        if ( die_pc_range( cu.root() ).contains(pc) ) {
        
            auto &lt = cu.get_line_table();
            auto it = lt.find_address(pc);
        
            if ( it == lt.end() ) {
                throw std::out_of_range{"Cannot find line entry"};
            }
            else {
                return it;
            }
        }
    }

    throw std::out_of_range{"Cannot find line entry"};
}

void MiniDbg::Debugger::print_source(const std::string& file_name, unsigned line, unsigned n_lines_context) {

    std::ifstream file {file_name};

    auto start_line = line <= n_lines_context ? 1 : line - n_lines_context;
    auto end_line = line + n_lines_context + (line < n_lines_context ? n_lines_context - line : 0) + 1;

    char c;
    auto current_line = 1u;
    
    while ( current_line != start_line && file.get(c) ) {
    
        if (c == '\n') {
            ++current_line;
        }
    }

    std::cout << (current_line == line ? "> " : "  ");

    while (current_line <= end_line && file.get(c)) {

        std::cout << c;

        if (c == '\n') {

            ++current_line;
            std::cout << (current_line==line ? "> " : "  ");
        }
    }

    std::cout << std::endl;
}

siginfo_t MiniDbg::Debugger::get_signal_info() {

    siginfo_t info;
    ptrace(PTRACE_GETSIGINFO, m_pid, nullptr, &info);
    return info;
}

void MiniDbg::Debugger::handle_sigtrap( siginfo_t info ) {

    switch ( info.si_code ) {
        
        case SI_KERNEL:
        case TRAP_BRKPT:
        {
            set_pc( get_pc() - 1 );

            std::cout << "Hit breakpoint at address 0x" << std::hex << get_pc() << std::endl;
            
            auto offset_pc = offset_load_address( get_pc() ); 
            auto line_entry = get_line_entry_from_pc( offset_pc );
            
            print_source( line_entry->file->path, line_entry->line );
            
            return;
        }
        case TRAP_TRACE:
            
            return;

        default:
            
            std::cout << "Unknown SIGTRAP code " << info.si_code << std::endl;
            return;
    }
}