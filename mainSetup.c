#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

pid_t child_pid = 1 ; // Global 

 
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */
    
    ct = 0;  
    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);  

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0){
		printf("\n");
		exit(0);  /* ^d was entered, end of user command stream */
		}
                  

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
		exit(-1);           /* terminate with error code of -1 */
		}

	printf(">>%s<<\n",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
	    case ' ':
	    case '\t' :               /* argument separators */
			if(start != -1){
				args[ct] = &inputBuffer[start];    /* set up pointer */
				ct++;
			}
			inputBuffer[i] = '\0'; /* add a null char; make a C string */
			start = -1;
			break;

		case '\n':                 /* should be the final char examined */
			if (start != -1){
				args[ct] = &inputBuffer[start];     
				ct++;
			}
			inputBuffer[i] = '\0';
			args[ct] = NULL; /* no more arguments to this command */
			break;

	    default :             /* some other character */
			if (start == -1)
				start = i;
			if (inputBuffer[i] == '&'){
			*background  = 1;
			inputBuffer[i-1] = '\0';
			
				}		
		 
			} /* end of switch */
			
		}    /* end of for */
		
	args[ct] = NULL; /* just in case the input line was > 80 */

	for (i = 0; i <= ct; i++)
		printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */

void kill_process(int pid){
	
	printf("Process termination operation\n");
	
    if (kill(pid, 0) == 0) {
    /* process is running or a zombie */
        kill(pid,SIGKILL);
        printf("process terminated \n");
    } else if (errno == ESRCH) {
    /* no such process with the given pid is running */
        printf("process not found \n");
    } else {
		printf("bir seklde terminate edildi\n");
    /* some other error... use perror("...") or strerror(errno) to report */
    }


}


int main(void)
{
	char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
	int background; /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2 + 1]; /*command line arguments */
	/**Background processleri nasl ele alacagini iyi dusun sıra orada !!! */
	

	while (1){

		if(child_pid != 0){ // If it is not child
			printf("\nanlk child_pid : %d benim pid : %d\n",child_pid,getpid());
			printf("myshell: ");
			fflush(0);
			background = 0;
			/*setup() calls exit() when Control-D is entered */
			setup(inputBuffer, args, &background);
			
			child_pid = fork(); // creating new child
		}
		
		
		if(child_pid > 0){ // If it is parent
			printf("I am parent : %d\n",getpid());
			
			if(background == 0){
				printf("not background processs\n");
				if(child_pid == wait(NULL)){
					printf("End of the parent 1 \n");
				 }
			 
			 }else{
				 
				 printf("Background Process!!\n");
				 printf("End of the parent 2 \n");
			  }
			
		}
		else if(child_pid == 0){ /* If it is child*/
			printf("I am child : %d\n",getpid());
			kill_process(getpid());
		}
		

		
		/** the steps are:
		(1) fork a child process using fork()
		(2) the child process will invoke execv()
		(3) if background == 0, the parent will wait,
		otherwise it will invoke the setup() function again. */
		}
				
			
			
    return 0;             

}
