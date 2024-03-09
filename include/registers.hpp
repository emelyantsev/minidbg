#ifndef MINIDBG_REGISTERS_HPP
#define MINIDBG_REGISTERS_HPP

#include <string>
#include <array>
#include <sys/user.h>


namespace MiniDbg {

    enum class Register {
        rax, 
        rbx, 
        rcx, 
        rdx,
        rdi, 
        rsi, 
        rbp, 
        rsp,
        r8,  
        r9,  
        r10, 
        r11,
        r12, 
        r13, 
        r14, 
        r15,
        rip, 
        rflags, 
        cs,
        orig_rax, 
        fs_base,
        gs_base,
        fs, 
        gs, 
        ss, 
        ds, 
        es
    };

    struct RegDescriptor {
        Register r;
        int dwarf_r;
        std::string name;
    };

    static const std::size_t n_registers = 27;

    extern const std::array<RegDescriptor, n_registers> g_register_descriptors;

    uint64_t get_register_value( pid_t pid, Register r ) ;

    void set_register_value( pid_t pid, Register r, uint64_t value ) ;

    uint64_t get_register_value_from_dwarf_register ( pid_t pid, unsigned regnum ) ;

    std::string get_register_name( Register r ) ;

    Register get_register_from_name( const std::string& name ) ;
}

#endif