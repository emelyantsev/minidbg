#ifndef MINIDBG_DEBUGGER_HPP
#define MINIDBG_DEBUGGER_HPP

#include <utility>
#include <string>
#include <linux/types.h>

namespace MiniDbg {

    class Debugger {
    
    public:

        Debugger(const std::string& prog_name, pid_t pid) : m_prog_name(prog_name), m_pid(pid) {}

        void Run();

    private:
    
        void handle_command(const std::string& line);
        void continue_execution();        

    private:    

        std::string m_prog_name;
        pid_t m_pid;
    };
}

#endif
