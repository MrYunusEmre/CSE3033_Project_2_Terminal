# CSE3033_Project_2_Terminal
# 1. OVERVIEW
*MyShell*, as its name reveals, is a **light customized Linux shell** which resembles **bash shell** in **Unix Platform**.
As a homework assigned in the **Operating Systems** course for junior majoring in CS,
it is an **individual group project** for **non-comercial** purposes.
This program allows you to interact with the Linux system with a command line parser.  

>Engineers :  
>>**Yunus Emre Ertunç**  
>>**Muhammed Enes Aktürk**  


# 2. FUNCTIONS
This shell support the following commands or operations:   

| Category | Commands |  
| :----------------: | :--------------------------------------|  
| basic iteration     | ps_all, search , bookmark             |  
| file system         | cd, dir, pwd,                         |
| process management  | &, exit, ctrl+z                       |
| io redirection      | <, >, >>, 2>, 2>>                     |  

# 3. USAGE
## How to build the project
In the Unix platform to compile and run that program write that statements :  

->  **gcc mainSetup.c -o mainSetup.o**  
-> **./mainSetup.o**  

## Some samples 

### Example  _execv()_ programs  

![](/gifs/vol3.gif)  

### **_ps\_all_** command :  
**_ps\_all_** - display the status of each background job (process).  
 It should be noted that the status information of
background jobs are displayed in two lists: list of background jobs still running, and the list
of background jobs that have finished execution since the last time the status is displayed
with the _ps\_all_ command.  

![](/gifs/vol1.gif)

### **search** command  
**_search_** - This command is very useful when you search a keyword or phrase in source
codes.The command takes a string that is going to be searched and searches this string in all
the files under the current directory and prints their line numbers, filenames and the line that
the text appears. If -r option is used, the command will recursively search all the
subdirectories as well. The file formats searched by the command are limited to .c, .C, .h,
and H.  

![](/gifs/vol2.gif)

### **bookmark** command 
**_bookmark_** - bookmark frequently used commands. You can type _bookmark -h_ to learn the usage of that command.  


![](/gifs/vol7.gif)

### **^Z** signal handling  
**_^Z_** - Stop the currently running foreground process, as well as any descendants of that
process.  

![](/gifs/vol4.gif)

### **exit** command 
**_exit_** - Terminate your shell process.  

![](/gifs/vol6.gif)

### **I/O Redirections**
The shell supports I/O-redirection on either or both stdin and/or stdout and it can include
arguments as well.  
Type **io -h** to see the usage of that command.  

![](/gifs/vol5.gif)


