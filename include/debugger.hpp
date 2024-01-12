#ifndef MINIDBG_DEBUGGER_HPP
#define MINIDBG_DEBUGGER_HPP

#include <utility>
#include <string>
#include <unordered_map>
#include <linux/types.h>


#include "breakpoint.hpp"

namespace MiniDbg {

    class Debugger {
    
    public:

        Debugger(const std::string& prog_name, pid_t pid) : m_prog_name( prog_name ), m_pid( pid ) {}

        void Run();

    private:
    
        void handle_command(const std::string& line);
        void continue_execution();
        void process_status(int status);
        void set_breakpoint_at_address(std::intptr_t addr);        

    private:    

        std::string m_prog_name;
        pid_t m_pid;
        std::unordered_map<std::intptr_t, Breakpoint> m_breakpoints;
    };
}

#endif
