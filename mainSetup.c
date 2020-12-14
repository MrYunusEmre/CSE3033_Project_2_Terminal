#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>


#include <limits.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
 
/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

int numOfArgs = 0; //
int processNumber = 1 ; //

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
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
	exit(-1);           /* terminate with error code of -1 */
    }

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
	numOfArgs = ct;
	for (i = 0; i <= ct; i++)
		printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */
 
 
pid_t parentPid ;


/*
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * */





/* int checkifexecutable(const char *filename)
 * 
 * Return non-zero if the name is an executable file, and
 * zero if it is not executable, or if it does not exist.
 */

int checkifexecutable(const char *filename)
{
     int result;
     struct stat statinfo;
     
     result = stat(filename, &statinfo);
     if (result < 0) return 0;
     if (!S_ISREG(statinfo.st_mode)) return 0;

     if (statinfo.st_uid == geteuid()) return statinfo.st_mode & S_IXUSR;
     if (statinfo.st_gid == getegid()) return statinfo.st_mode & S_IXGRP;
     return statinfo.st_mode & S_IXOTH;
}


/* int findpathof(char *pth, const char *exe)
 *
 * Find executable by searching the PATH environment variable.
 *
 * const char *exe - executable name to search for.
 *       char *pth - the path found is stored here, space
 *                   needs to be available.
 *
 * If a path is found, returns non-zero, and the path is stored
 * in pth.  If exe is not found returns 0, with pth undefined.
 */

int findpathof(char *pth, const char *exe)
{
     char *searchpath;
     char *beg, *end;
     int stop, found;
     int len;

     if (strchr(exe, '/') != NULL) {
	  if (realpath(exe, pth) == NULL) return 0;
	  return  checkifexecutable(pth);
     }

     searchpath = getenv("PATH");
     if (searchpath == NULL) return 0;
     if (strlen(searchpath) <= 0) return 0;

     beg = searchpath;
     stop = 0; found = 0;
     do {
	  end = strchr(beg, ':');
	  if (end == NULL) {
	       stop = 1;
	       strncpy(pth, beg, PATH_MAX);
	       len = strlen(pth);
	  } else {
	       strncpy(pth, beg, end - beg);
	       pth[end - beg] = '\0';
	       len = end - beg;
	  }
	  if (pth[len - 1] != '/') strncat(pth, "/", 2);
	  strncat(pth, exe, PATH_MAX - len);
	  found = checkifexecutable(pth);
	  if (!stop) beg = end + 1;
     } while (!stop && !found);
	  
     return found;
}





/*
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * */


struct listProcess{
	
	int processNumber ;
	pid_t pid ;	     // pid
	char progName[50] ; // program name
	struct listProcess *nextPtr ;
	
};

typedef struct listProcess ListProcess ; //synonym for struct listProcess
typedef ListProcess *ListProcessPtr; //synonym for ListProcess*


void insert(ListProcessPtr *sPtr , pid_t pid , char progName[]){
	
	ListProcessPtr newPtr = malloc(sizeof(ListProcess)); // Create Node
	
	if(newPtr != NULL){
		strcpy(newPtr->progName, progName);
		newPtr->processNumber = processNumber ;
		newPtr->pid = pid;
		newPtr->nextPtr = NULL;
		
		ListProcessPtr previousPtr = NULL;
		ListProcessPtr currentPtr = *sPtr;
		
		while(currentPtr != NULL){
			previousPtr = currentPtr;
			currentPtr = currentPtr->nextPtr;
		}
		
		if(previousPtr == NULL){ //insert to the beginning
			newPtr->nextPtr = *sPtr;
			*sPtr = newPtr;
		}else{
			previousPtr->nextPtr = newPtr;
			newPtr->nextPtr = currentPtr;
			
		}		
	}
	else{
		printf("No memory available\n");
	}
	
	
	
		
}

int isEmpty(ListProcessPtr sPtr){return sPtr == NULL;}

void printList(ListProcessPtr currentPtr){
		
	int status ; 
	ListProcessPtr tempPtr = currentPtr;
	if(isEmpty(currentPtr)) puts("List is empty\n");
	else{
		
		puts("Running : ");
		while(tempPtr != NULL){
			if(waitpid(tempPtr->pid,&status,WNOHANG)==0)
				printf("\t[%d] %s (Pid=%ld)\n",tempPtr->processNumber,tempPtr->progName,(long)(tempPtr->pid));
			tempPtr = tempPtr->nextPtr;
		}
		puts("Stopped : ");
		while(currentPtr != NULL){
			if(waitpid(currentPtr->pid,&status,WNOHANG)==-1)
				printf("\t[%d] %s (Pid=%ld)\n",currentPtr->processNumber,currentPtr->progName,(long)(currentPtr->pid));
			currentPtr = currentPtr->nextPtr;
		}
		
		puts("\n");
		
		}
	
}





void childSignalHandler(int signum) {
	int status;
	pid_t pid;
	
	pid = waitpid(-1, &status, WNOHANG);
}

pid_t childPid ;

void partA(char inputBuffer[], char *args[],int *background,ListProcessPtr *sPtr){
	
	
	childPid = fork();
	
	
	 char path[PATH_MAX+1];
	 char *progpath = strdup(args[0]);
	 char *prog = basename(progpath);
	 char *exe;
	 
	//if(strcmp('ps_all',args[0]) == 0) printList(*sPtr);
	
	
	if(childPid < 0) printf("Error");
	else if(childPid == 0 && *background == 0){//create foreground process
		
				
		//printf("%ld : I am child foreground\n",(long)getpid());	
		
		exe=args[0];
		
		if (!findpathof(path,exe)) {
			printf("No executable \"%s\" found\n", exe);
		 }
		 
		 //puts(path);
		 free(progpath);
		 

				
		execv(path,args);
		
		
		
			
	}
	else if(childPid == 0 && *background == 1){//create background process
		
		//printf("%ld : I am child background \n",(long)getpid());	
		
		exe=args[0];
		if (!findpathof(path, exe)) {
			printf("No executable \"%s\" found\n", exe);
		 }
		 //puts(path);
		free(progpath);

		
		if(*args[numOfArgs-1] == '&')// If last argument is &, delete it
			args[numOfArgs-1] = '\0';
			
		
		
		execv(path,args);
		
		}
		
		
	else{ //Parent part
		
		//printf("%ld : I am parent\n",(long)getpid());
		
		if(*background==0)
			wait(NULL);
			
		else{
			
			insert(&(*sPtr),childPid,args[0]);
			processNumber++;
			
			}		
			
	}
	
}

 
int main(void)
{
	
	char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
	int background; /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2 + 1]; /*command line arguments */
	
	signal(SIGCHLD, childSignalHandler);
	parentPid = getpid();
	
	ListProcessPtr startPtr = NULL; //starting pointer

	//printf("%ld : I am parentpid\n%ld : I am getpid\n",(long)parentPid,(long)getpid());
	while (parentPid==getpid()){
				background = 0;
				//printf("maindeki id : %d\n",getpid());
				if(isEmpty(startPtr))		processNumber=1;
				printf("myshell: ");
				fflush(0);
				/*setup() calls exit() when Control-D is entered */
				setup(inputBuffer, args, &background);
				if(strcmp(args[0],"ps_all")==0) 
					printList(startPtr); //You need to press ps_all to see the content of list 
				else
					partA(inputBuffer, args, &background,&startPtr);
	}   
        

				/** the steps are:
				(1) fork a child process using fork()
				(2) the child process will invoke execv()
				(3) if background == 0, the parent will wait,
				
				otherwise it will invoke the setup() function again. */
	
}
