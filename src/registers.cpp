#include "registers.hpp"

#include <sys/ptrace.h>
#include <algorithm>
#include <stdexcept>


namespace MiniDbg {


const std::array<RegDescriptor, n_registers> g_register_descriptors {

    {
        { Register::r15, 15, "r15" },
        { Register::r14, 14, "r14" },
        { Register::r13, 13, "r13" },
        { Register::r12, 12, "r12" },
        { Register::rbp, 6, "rbp" },
        { Register::rbx, 3, "rbx" },
        { Register::r11, 11, "r11" },
        { Register::r10, 10, "r10" },
        { Register::r9, 9, "r9" },
        { Register::r8, 8, "r8" },
        { Register::rax, 0, "rax" },
        { Register::rcx, 2, "rcx" },
        { Register::rdx, 1, "rdx" },
        { Register::rsi, 4, "rsi" },
        { Register::rdi, 5, "rdi" },
        { Register::orig_rax, -1, "orig_rax" },
        { Register::rip, -1, "rip" },
        { Register::cs, 51, "cs" },
        { Register::rflags, 49, "eflags" },
        { Register::rsp, 7, "rsp" },
        { Register::ss, 52, "ss" },
        { Register::fs_base, 58, "fs_base" },
        { Register::gs_base, 59, "gs_base" },
        { Register::ds, 53, "ds" },
        { Register::es, 50, "es" },
        { Register::fs, 54, "fs" },
        { Register::gs, 55, "gs" },
    }
};


uint64_t get_register_value( pid_t pid, Register r ) {
    
    user_regs_struct regs;

    ptrace( PTRACE_GETREGS, pid, nullptr, &regs );

    auto it = std::find_if( std::begin( g_register_descriptors ), std::end( g_register_descriptors ),
                            [ r ]( const RegDescriptor& rd ) { return rd.r == r; } );

    return *( reinterpret_cast< uint64_t* >( &regs ) + ( it - std::begin( g_register_descriptors ) ) );
}


void set_register_value( pid_t pid, Register r, uint64_t value ) {

    user_regs_struct regs;
    ptrace( PTRACE_GETREGS, pid, nullptr, &regs );
    
    auto it = std::find_if( std::begin( g_register_descriptors ), std::end( g_register_descriptors ),
                            [r](const RegDescriptor& rd) { return rd.r == r; });

    *(reinterpret_cast< uint64_t* >( &regs ) + (it - std::begin( g_register_descriptors ) ) ) = value;
    ptrace( PTRACE_SETREGS, pid, nullptr, &regs );
}


uint64_t get_register_value_from_dwarf_register ( pid_t pid, unsigned regnum ) {

    auto it = std::find_if( std::begin( g_register_descriptors ), std::end( g_register_descriptors ), 
                            [ regnum ]( const RegDescriptor& rd ) { return rd.dwarf_r == regnum; } );
    
    if ( it == std::end( g_register_descriptors ) ) {
        
        throw std::out_of_range( "Unknown dwarf register" );
    }

    return get_register_value( pid, it->r );
}

std::string get_register_name( Register r ) {

    auto it = std::find_if( std::begin( g_register_descriptors ), std::end( g_register_descriptors ),
                            [ r ]( const RegDescriptor& rd ) { return rd.r == r; });
    return it->name;
}

Register get_register_from_name( const std::string& name ) {

    auto it = std::find_if( std::begin( g_register_descriptors ), std::end( g_register_descriptors ),
                            [ name ]( const RegDescriptor& rd) { return rd.name == name; });
    return it->r;
}

}