# Debugger 

## Types
> **error class**
error class to handle exeptions with two main function which do no return to normal control flow  
1. **send:**  throws any custom error 
2. **send_errno:** handles error which are occured by system

> **process_state**
Keeps track of state of the process

> **stop_reason**
contains reason if program stopped or terminated

> **process**
Contains info abt the process and programs to handle all the functions related to a process
1. wait_on_signal
2. launch
3. attach
4. resume


## Top to down shows the execution flow of the program

> **/tools/pdb.cpp**  
Entry point of the whole program where we attach the existing inferior or new inferior to parent and then proceed to run the debugger  

> **attach()**  
Takes the argc and argv and then proceed to execute depending on the existing process or executes new one  
attaches to existing one by proces::attach and returns the pointer to process type  
launches new by process::launch     

> **handle_command()**




## NOTES
### Register Info
We want data about all 124 registers in two
separate places, register_id and g_register_infos. Keeping this much data in sync can become a nightmare, as it’s easy to make a mistake that we can’t find at compile-time without more machinery. Unless we have adequate test coverage, these issues can hide in the darkness, hitting production when we
least expect it. Fortunately, tools called X-Macros solve exactly this problem without the need for an external code generator.
X-Macros allow us to maintain independent data structures whose members
or operations rely on the same underlying data and must be kept in sync.

### X-Macros
X-Macros allow us to maintain independent data structures whose members or operations rely on the same underlying data and must be kept in sync.

> **registers.inc**  
contains X-macro for register info 
We give GPR_OFFSET a register name. It gets the offset of the regs member inside user, then adds to it the offset of the given register user_regs_struct

The macros which take the name of the sub-register and the name of the register that they’re a part of (super). Sub-registers don’t have DWARF IDs, so we use a dummy value of -1 in this field. In most cases, we use the offset of the super-register as the register offset. This approach is correct because x64 is little-endian, so the first byte of the register will always be stored at the lowest address. However, for the high 8-bit sub-registers, we need to add 1 to this value to access the second byte of the super-register

we also define the orig_rax which provides a way to get id of a syscall

The register numbers for the consecutive st, mm, and xmm registers are also consecutive, so we calculate them by adding the register number to the DWARF ID for the lowest register. Note that, while the st registers are 10 bytes wide in hardware, they’re stored as 16-byte values in the user_fpregs_struct type, so we supply 16 as their size . Similarly, while the mm registers are 8 bytes wide, they have an additional 8 bytes of padding in user_fpregs_struct, so we multiply their offset by 16 rather than 8.