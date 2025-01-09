# Debugger 

Top to down shows the execution flow of the program

> **/tools/pdb.cpp**  
we attach to the given program or process.  

> **attach()**  
 Takes the pid and depending upon the type of argument given it either attaches to a existing process or runs the new program in diff process.  
 In case of new program we call fork which copies the current process and starts  running  
 Here depending upon if the process is the parent(pid returned is pid of child) or child(pid returned is zero) we conitnue to different workflows.  
 In child process we call execlp and run the program.  

> **Back to main(/tools/pdb.cpp)**
  we wait for the process attached to stop by waitpid  
  we use readline function to give the tui  
  we read each line as the command and then handle its execution  
  we also store each command used in history_list provided by readline library and execute them if empty command is given  

>> **handle_command()**



