#ifndef MINIDBG_BREAKPOINT_HPP
#define MINIDBG_BREAKPOINT_HPP

#include <cstdint>
#include <sys/types.h>
#include <sys/ptrace.h>

namespace MiniDbg {
    
    class Breakpoint {
    
    public:

        Breakpoint( pid_t pid, std::intptr_t addr ) : m_pid( pid ), m_addr( addr ), m_enabled( false ), m_saved_data( 0 ) {}

        void Enable() {
        
            long data = ::ptrace( PTRACE_PEEKDATA, m_pid, m_addr, nullptr );
            m_saved_data = static_cast<uint8_t>( data & 0xFF ); //save bottom byte
            
            long int3 = 0xCC;
            long data_with_int3 = ( ( data & ~0xFF ) | int3 ); //set bottom byte to 0xcc

            ::ptrace( PTRACE_POKEDATA, m_pid, m_addr, data_with_int3 );
            m_enabled = true;
        }

        void Disable() {

            long data = ::ptrace( PTRACE_PEEKDATA, m_pid, m_addr, nullptr );
            long restored_data = ( ( data & ~0xFF )  | m_saved_data );
            ::ptrace( PTRACE_POKEDATA, m_pid, m_addr, restored_data );
            m_enabled = false;
        }

        bool is_enabled() const { return m_enabled; }      
        std::intptr_t get_address() const { return m_addr; }
    
    private:

        pid_t m_pid;
        std::intptr_t m_addr;
        bool m_enabled;
        uint8_t m_saved_data; //data which used to be at the breakpoint address
    };
}

#endif

