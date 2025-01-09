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