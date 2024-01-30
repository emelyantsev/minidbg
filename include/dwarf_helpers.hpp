#ifndef DWARF_HELPERS_HPP
#define DWARF_HELPERS_HPP


#include <sys/ptrace.h>
#include "dwarf/dwarf++.hh"
#include "registers.hpp"
#include <iostream>

class ptrace_expr_context : public dwarf::expr_context {

    public:

        ptrace_expr_context( pid_t pid, uint64_t load_address ) : m_pid( pid ), m_load_address( load_address ) {}

        dwarf::taddr reg( unsigned regnum ) override {

            //std::cerr << "reg" << std::dec << regnum << std::endl;

            return MiniDbg::get_register_value_from_dwarf_register( m_pid, regnum );
        }

        dwarf::taddr pc() override {

            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, m_pid, nullptr, &regs);

            dwarf::taddr temp = regs.rip - m_load_address ;

            //std::cerr << std::hex << temp << std::endl;

            return regs.rip - m_load_address;
        }

        dwarf::taddr deref_size (dwarf::taddr address, unsigned size) override {

            //std::cerr << "deref_size" << std::hex << address << std::endl;
            
            return ptrace( PTRACE_PEEKDATA, m_pid, address + m_load_address, nullptr );
        }

    private:

        pid_t m_pid;
        uint64_t m_load_address;
};

template class std::initializer_list<dwarf::taddr>;

#endif