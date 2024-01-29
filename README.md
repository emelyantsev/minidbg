### Writing a Linux debugger

1. [Setup](https://blog.tartanllama.xyz/writing-a-linux-debugger-setup/)
2. [Breakpoints](https://blog.tartanllama.xyz/writing-a-linux-debugger-breakpoints/)
3. [Registers and memory](https://blog.tartanllama.xyz/writing-a-linux-debugger-registers/)
4. [Elves and dwarves](https://blog.tartanllama.xyz/writing-a-linux-debugger-elf-dwarf/)
5. [Source and signals](https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/)
6. [Source-level stepping](https://blog.tartanllama.xyz/writing-a-linux-debugger-dwarf-step/)
7. [Source-level breakpoints](https://blog.tartanllama.xyz/writing-a-linux-debugger-source-break/)
8. [Stack unwinding](https://blog.tartanllama.xyz/writing-a-linux-debugger-unwinding/)
___

#### Usage:

```
./minidbg <program name>

cont -> continue

break <OxADDRESS>
      <line>:<filename>  
      <function_name>  

register <dump>
register <read> <register_name>
register <write> <register_name> <0xVALUE>

memory <read> <0xADDRESS>
memory <write> <0xADDRESS> <0xVALUE>

step
stepi
next
finish

symbol <symbol_name>

Ctrl+D  -> exit

```