### Writing a minimalistic Linux debugger

1. [Setup](https://blog.tartanllama.xyz/writing-a-linux-debugger-setup/)
2. [Breakpoints](https://blog.tartanllama.xyz/writing-a-linux-debugger-breakpoints/)
3. [Registers and memory](https://blog.tartanllama.xyz/writing-a-linux-debugger-registers/)
4. [Elves and dwarves](https://blog.tartanllama.xyz/writing-a-linux-debugger-elf-dwarf/)
5. [Source and signals](https://blog.tartanllama.xyz/writing-a-linux-debugger-source-signal/)

____________________________________________________________________

#### Usage:

```
./minidbg <program name>

cont   [continue]

break <OxADDRESS>

register <dump>
register <read> <register_name>
register <write> <register_name> <0xVALUE>

memory <read> <0xADDRESS>
memory <write> <0xADDRESS> <0xVALUE>

Ctrl+D   [exit]

```