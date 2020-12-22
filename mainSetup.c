#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <string.h>

#include <dirent.h> 
#include <stdbool.h>
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
 
#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

#define READ_FLAGS (O_RDONLY)
#define CREATE_FLAGS (O_WRONLY | O_CREAT | O_TRUNC)
#define APPEND_FLAGS (O_WRONLY | O_CREAT | O_APPEND)
#define READ_MODES (S_IRUSR | S_IRGRP | S_IROTH)
#define CREATE_MODES (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


char inputFileName[20]; 
char outputFileName[20];
char outputRedirectSymbol[3] = {"00"};

int inputRedirectFlag;
int outputRedirectFlag;

int numOfArgs = 0; //
int processNumber = 1 ; //

pid_t parentPid ; // stores the parent pid



/*
	
	SATIR 241 -> BOOKMARK INSERT 
	SATIR 421 -> BOOKMARK CALISTIRMA YERI KESIN BAKMAN GEREKIYOR YAPAMADIM

	550 den 750 ye kadar ben yazdım bi de 241 in oralarda bookmarka struct oluşturdum insert delete isempty run fonksyionlarını olusturdum 

*/






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

} /* end of setup routine */
 

// next 2 functions is looking PATH and find executable input  

/* int checkifexecutable(const char *filename)
 * 
 * Return non-zero if the name is an executable file, and
 * zero if it is not executable, or if it does not exist.
 */


int formatOutputSymbol(char *arg){
	if(strcmp(arg, ">") == 0)
		return 0;
	else if(strcmp(arg, ">>") == 0)
		return 1;
	else if(strcmp(arg, "2>") == 0)
		return 2;

	return -1;

}

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


struct listProcess{
	
	int processNumber ;
	pid_t pid ;	     // pid
	char progName[50] ; // program name
	struct listProcess *nextPtr ;
	
};

struct bookmark{

	char progName[50] ; // program name which added into bookmark
	struct bookmark *nextPtr ;

};

typedef struct listProcess ListProcess ; //synonym for struct listProcess
typedef ListProcess *ListProcessPtr; //synonym for ListProcess*
typedef struct bookmark bookmarks ; //synonym for struct bookmark
typedef bookmarks *bookmarkPtr ; //synonym for struct bookmarks

pid_t fgProcessPid = 0;

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

// inserting program into bookmark struct
void insertBookmark(bookmarkPtr *bPtr , char progName[]){

	bookmarkPtr newPtr = malloc(sizeof(bookmarks));

	if(newPtr != NULL){
		strcpy(newPtr->progName, progName);
		newPtr->nextPtr = NULL;

		bookmarkPtr previousPtr = NULL ;
		bookmarkPtr currentPtr = *bPtr ;

		while(currentPtr != NULL){
			previousPtr = currentPtr ;
			currentPtr = currentPtr->nextPtr ;
		}

		if(previousPtr == NULL){ //insert to the beginning
			newPtr->nextPtr = *bPtr;
			*bPtr = newPtr;
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

// isempty for bookmark struct
int isEmptyBookmark(bookmarkPtr bPtr){return bPtr == NULL;}

void printList(ListProcessPtr currentPtr){
		
	int status ; 
	ListProcessPtr tempPtr = currentPtr;
	if(isEmpty(currentPtr)) puts("List is empty\n");
	else{
		
		puts("Running : "); 			// prints the running processes
		while(tempPtr != NULL){				// looks all processes in linkedlist and controls if they are running or has stopped
			if(waitpid(tempPtr->pid,&status,WNOHANG)==0)		//if process is working , this waitpid will return 0
				printf("\t[%d] %s (Pid=%ld)\n",tempPtr->processNumber,tempPtr->progName,(long)(tempPtr->pid));
			tempPtr = tempPtr->nextPtr;
		}
		puts("Finished : ");			// prints the finished processes
		while(currentPtr != NULL){			// looks all processes in linkedlist and controls if they are running or has stopped
			if(waitpid(currentPtr->pid,&status,WNOHANG)==-1)		//if process has stopped , this waitpid will return -1
				printf("\t[%d] %s (Pid=%ld)\n",currentPtr->processNumber,currentPtr->progName,(long)(currentPtr->pid));
			currentPtr = currentPtr->nextPtr;
		}
		
		puts("\n");
		
		}
	
}

void printListBookmark(bookmarkPtr bPtr){

	int i=0 ;
	bookmarkPtr tempPtr = bPtr ;
	if (isEmptyBookmark(bPtr)) puts("List is empty\n");
	else{
		while(tempPtr->nextPtr != NULL){
			printf("%d %s\n",i,tempPtr->progName);
			i++;
			tempPtr = tempPtr->nextPtr ;
		}
		printf("%d %s\n",i,tempPtr->progName);
	}

}

void deleteStoppedList(ListProcessPtr *currentPtr){
	
	int status ;
	
	if((*currentPtr)==NULL)
		return ;
		
	if(waitpid((*currentPtr)->pid,&status,WNOHANG)==-1){
		// if the stopped process is the first 
	
		ListProcessPtr tempPtr = *currentPtr ;
		*currentPtr = (*currentPtr)->nextPtr ;
		free(tempPtr) ;
		deleteStoppedList(currentPtr);
	}
	else{
		// if the stopped process is not the first
		 
		ListProcessPtr previousPtr = *currentPtr ;
		ListProcessPtr tempPtr = (*currentPtr)->nextPtr ;
		
		while(tempPtr!=NULL && waitpid(tempPtr->pid,&status,WNOHANG)!=-1){
			previousPtr = tempPtr;
			tempPtr=tempPtr->nextPtr;
		}
		if(tempPtr!=NULL){
			ListProcessPtr delPtr = tempPtr ;
			previousPtr->nextPtr=tempPtr->nextPtr;
			free(delPtr);
			deleteStoppedList(currentPtr);
		}
			
	}
	
}

void deleteBookmarkList(char *charindex,bookmarkPtr *currentPtr){

	int index = atoi(charindex);

	if(isEmptyBookmark(*currentPtr))
		printf("List is empty\n");
	else{

		// delete first item
		if(index==0){
			bookmarkPtr tempPtr = *currentPtr;
			*currentPtr=(*currentPtr)->nextPtr;
			free(tempPtr);
		}
		else{	// delete others
			bookmarkPtr previousPtr = *currentPtr ;
			bookmarkPtr tempPtr = (*currentPtr)->nextPtr ;
			int temp = 1;

			while(temp!=index && tempPtr!=NULL){
				previousPtr = tempPtr ;
				tempPtr = tempPtr->nextPtr ;
				temp++;
			}
			if(tempPtr==NULL)
				printf("There is no bookmark with this index.\n");
			else{

				bookmarkPtr delPtr = tempPtr ;
				previousPtr->nextPtr=tempPtr->nextPtr;
				free(delPtr);

			}

		}

	}

}

void runBookmarkIndex(char *charindex, bookmarkPtr currentPtr){

	int index = atoi(charindex);
	char *progpath ;

	if(isEmptyBookmark(currentPtr))
		printf("List is empty\n");
	else{

		bookmarkPtr tempPtr = currentPtr;
		int j=0 ;
		while(tempPtr!=NULL && j!=index){
			tempPtr=tempPtr->nextPtr;
			j++;
		}
		if (tempPtr==NULL)
		{
			printf("There is no bookmark in this index.\n");
		}
		else{

			char exe[MAX_LINE] ;
			strcpy(exe,tempPtr->progName);
			int length = strlen(exe);
			int i = 0;
			exe[length - 2] = '\0';
			
			for(i = 0 ; i < length; i++){
				exe[i] = exe[i+2];
			}

			char command[100];
			sprintf(command, "%s",exe);
			system(command);

		}

	}

}

int killAllChildProcess(pid_t ppid)
{
   char *buff = NULL;
   size_t len = 255;
   char command[256] = {0};

   sprintf(command,"ps -ef|awk '$3==%u {print $2}'",ppid);
   FILE *fp = (FILE*)popen(command,"r");
   while(getline(&buff,&len,fp) >= 0)
   {
 	 killAllChildProcess(atoi(buff));
	 kill(atoi(buff),SIGKILL);
   }
   free(buff);
   fclose(fp);
   return 0;
}


void childSignalHandler(int signum) {
	int status;
	pid_t pid;
	
	
	pid = waitpid(-1, &status, WNOHANG);
}


void sigtstpHandler(){ //When we press ^Z, this method will be invoked automatically
	
	if(fgProcessPid == 0 || waitpid(fgProcessPid,NULL,WNOHANG) == -1){
		system("clear");
		printf("myshell: ");
		fflush(stdout);
		return;
	}


	killAllChildProcess(fgProcessPid);

	kill(fgProcessPid,SIGKILL);
	fgProcessPid = 0;
}



void parentPart(char *args[], int *background , pid_t childPid , ListProcessPtr *sPtr){

	if(*background == 1){ //Background Process

		waitpid(childPid, NULL, WNOHANG);
		setpgid(childPid, childPid); // This will put that process into its process group
		insert(&(*sPtr),childPid,args[0]);
		processNumber++;

	}else{ // Foreground Process

		setpgid(childPid, childPid); // This will put that process into its process group
		fgProcessPid = childPid;

		 if(childPid != waitpid(childPid, NULL, WUNTRACED))
            perror("Parent failed while waiting the child due to a signal or error!!!");

	}

}

void inputRedirect(){

	int fdInput;

	if(inputRedirectFlag == 1){///This is for getting input from file
		fdInput = open(inputFileName, READ_FLAGS, READ_MODES);

		if(fdInput == -1){
	        perror("Failed to open the file given as input...");
	        return;
	    }

	    if(dup2(fdInput, STDIN_FILENO) == -1){
	        perror("Failed to redirect standard input...");
	        return;
	    }

	    if(close(fdInput) == -1){
	        perror("Failed to close the input file...");
	        return;
	    }

	}

}
void outputRedirect(){

	int fdOutput;

			// > is 0 | >> is 1 | 2> is 2
		int outputMode = formatOutputSymbol(outputRedirectSymbol);

		if(outputMode == 0){ // For > part

			fdOutput = open(outputFileName, CREATE_FLAGS, CREATE_MODES);

	        if(fdOutput == -1){ 
        		printf("PART 1\n");
         	   perror("Failed to create or append to the file given as input...");
         	   return;
        	}


        	if(dup2(fdOutput, STDOUT_FILENO) == -1){
				perror("Failed to redirect standard output...");
				return;
			}

		}
		else if(outputMode == 1){ // for >> part

			fdOutput = open(outputFileName, APPEND_FLAGS, CREATE_MODES);

	        if(fdOutput == -1){ ///
        		printf("PART 1\n");
         	   perror("Failed to create or append to the file given as input...");
         	   return;
        	}


        	if(dup2(fdOutput, STDOUT_FILENO) == -1){
				perror("Failed to redirect standard output...");
				return;
			}

		}
		else{	// for 2> part
			fdOutput = open(outputFileName, CREATE_FLAGS, CREATE_MODES);

			if(fdOutput == -1){
				printf("PART 1\n");
				perror("Failed to create or append to the file given as input...");
				return;
     	   }

     	   if(dup2(fdOutput, STDERR_FILENO) == -1){
				perror("Failed to redirect standard error...");
				return;
			}

		}

}

void childPart(char path[], char *args[]){

	int fdInput;
	int fdOutput;

	if(inputRedirectFlag == 1){ //This is for myshell: myprog [args] < file.in
		inputRedirect();

		if(outputRedirectFlag == 1){ // This is for myprog [args] < file.in > file.out
			outputRedirect();
		}

	}else if(outputRedirectFlag == 1){ // This is for myprog [args] > file.out and myshell: myprog [args] >> file.out and myshell: myprog [args] 2> file.out
		outputRedirect();
	}

	execv(path,args);


}

void createProcess(char path[], char *args[],int *background,ListProcessPtr *sPtr){

	pid_t childPid ;

	childPid = fork();


	if(childPid == -1){perror("fork() function is failed!\n"); return;}
	else if(childPid != 0){ // Parent Part
		parentPart(args , &(*background),childPid , &(*sPtr));

	}else{	//Child Part
		childPart(path , args);

	}


}

//   ---- ayrica bookmark kontrolde de "ls -la" gibi inputlar icin " ile basladigini veya " ile bittigini kontrol etmek icin kullanıyorum
int startsWith(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
           lenstr = strlen(str);
    return lenstr < lenpre ? 0 : memcmp(pre, str, lenpre) == 0;

}


/*
*	Return if parameter pre ends with suffix or not
*   fonksiyona current directorydeki tüm dosyalar pre olarak geliyor , fonksiyon bu file .c .... ile mi bitiyor diye kontrol ediyor
*   ---- ayrica bookmark kontrolde de "ls -la" gibi inputlar icin " ile basladigini veya " ile bittigini kontrol etmek icin kullanıyorum
*/
int endsWith(const char *pre, const char *suffix){
    if (!pre || !suffix)
        return 0;
    size_t lenstr = strlen(pre);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(pre + lenstr - lensuffix, suffix, lensuffix) == 0;

}

void printBookmarkUsage(){
	printf("*Bookmark Usage : \n->\"bookmark -l\" to see the bookmark list.\n->\"bookmark -i (index)\" to run the bookmark index.\n->\"bookmark -d (index)\" to delete the item from bookmark.\n");
}

int isInteger(char arg[]){
	int length , i;
	length = strlen (arg);
    for (i=0;i<length; i++)
        if (!isdigit(arg[i]))
        {
            printf("Please check your arguments !\n");
            printBookmarkUsage();
            return 1;
        }
	return 0;
}

void bookmarkCommand(char *args[], bookmarkPtr *startPtrBookmark){

	int i=0; 
	while(args[i] != NULL){
		i++;
	}

	if(i==1){
		printf("Wrong usage of Bookmark! You can type \"bookmark -h\" to see the correct usage.\n");
		return;
	}
	else if((strcmp(args[1],"-h")==0) && i==2){ 
		printBookmarkUsage();
		return;
	}		
	else if((strcmp(args[1],"-l")==0) && i==2){

		printListBookmark(*startPtrBookmark);

	}
	else if((strcmp(args[1],"-i")==0) && i==3){

		if(isInteger(args[2]) == 0){
			runBookmarkIndex(args[2],*startPtrBookmark);
			return;
		}else{
			return;
		}	

	}
	else if((strcmp(args[1],"-d")==0) && i==3){

		if(isInteger(args[2]) == 0){
			deleteBookmarkList(args[2],startPtrBookmark);
			return;
		}	
		else{
			return;
		}

	}
	else if(startsWith("\"",args[1])){


		// EGER BOOKMARKIN ICINE GECERLI OLMAYAN BIR ISLEM KOYMAYALIM SALLAYAMASIN ISLEMLERI DIYORSAN
		// KONTROLU BURADA YAPALIM HA YOK DIYOSAN SAL


		if(endsWith(args[1],"\"") == 0){
			printf("Wrong usage of Bookmark! You can type \"bookmark -h\" to see the correct usage.\n");
			return;
		}

		int j=1 ;
		while(!endsWith(args[j],"\"")){

			j++;
		}

		if(j != i-1){
			printf("Wrong usage of Bookmark! You can type \"bookmark -h\" to see the correct usage.\n");
			return;
		}
			
		else{
			char exe[MAX_LINE] ;
			int k=1 ;
			while(k<=j){
				strcat(exe,args[k]);
				strcat(exe," ");
				k++;
			}
			insertBookmark(startPtrBookmark,exe);
		}

	}
	else{
		printf("Wrong usage of Bookmark! You can type \"bookmark -h\" to see the correct usage.\n");
		return;
	}

}


///ŞİMDİLİK GEREKSİZ 
/*

// searchdan 2 parametre ile çocuk oluşturup childPartı çağırmam lazımdı o yüzden böyle yaptım kanka 
void createProcess2(char path[], char *args[]){

	pid_t childPid ;

	childPid = fork();


	if(childPid == -1){perror("fork() function is failed!\n"); return;}
	else if(childPid != 0){ // Parent Part
		

		wait(NULL);
	}else{	//Child Part
		childPart(path , args);

	}

}
*/
///ŞİMDİLİK GEREKSİZ 
/*
void findPattern(char *pattern , char *fileName){

	char *argv[] = { "/usr/bin/grep", "-n", pattern , fileName , NULL };
	createProcess2(argv[0],argv);

}
*/


void clearLine(char args[],char lineNumber[]){
	int i = 0;

	int length = strlen(args);
	int digitNum = 1;

	for(i = 0 ; i < length; i++){
		if(isdigit(args[i])){ //This will determine the number of digits
			lineNumber[i] = args[i];
			digitNum++;
		}
		else break;
	}

	for(i = 0; i < length; i++){
		args[i] = args[digitNum + i];
	}

}

/**
This function takes fileName and pattern arguments. 
Determines the file name , line number and line by using grep.
Then combines all of them and print.

*/
void printSearchCommand(char *fileName , char *pattern){

	char file[1000] = {0};

	char *buff = NULL;
	char fName[256] = {0};
	size_t len = 255;

	sprintf(fName,"grep -rnwl  %s -e %s | awk '{print $0}'",fileName, pattern);
	FILE *fp = (FILE*)popen(fName,"r");

	while(getline(&buff,&len,fp) >= 0)
	{
		strcpy(file,buff);
	}
	free(buff);
	fclose(fp);
	char allLine[256] = {0};

	char *buff2 = NULL;
	char command[256] = {0};
	size_t len2 = 255;

	sprintf(command,"grep -rnw  %s -e %s | awk '{print $0}'",fileName , pattern);

	FILE *fp2 = (FILE*)popen(command,"r");
	while(getline(&buff2,&len2,fp2) >= 0)
	{
		strcpy(allLine,buff);
	}
	free(buff2);
	fclose(fp2);

	char lineNumber[15] = {0};

	clearLine(allLine,lineNumber);

	if(strlen(file) < 1  || !isdigit(lineNumber[0])){

	}else{
		file[strlen(file)-1] = '\0';

		printf("%s : %s -> %s",lineNumber,file , allLine);
	}


}


/**
 * Lists all files and sub-directories recursively 
 * considering path as base path.
 * Fonksiyon current directorydeki tüm directorylere recursive olarak giriyor ve tüm dosyaları basıyor , bunu değişicez return yapıcaz
 * sonra endswithe yollayıp sadece istediğimiz dosyaları alıcaz ve searchCommanda dönücez bunu
 */

void listFilesRecursively(char *basePath,char *pattern)
{
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);

    // Unable to open directory stream
    if (!dir)
        return;
  

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {	

        	char fName[50];
          
       	   strcpy(fName,dp->d_name);


       	   char grepFile[1000];
       	   strcpy(grepFile , basePath);
       	   strcat(grepFile , "/");
       	   strcat(grepFile , dp->d_name);

       	   if(fName[strlen(fName) - 2] == '.' && (fName[strlen(fName) - 1] == 'c' || fName[strlen(fName) - 1] == 'C' ||
       	   		 fName[strlen(fName) - 1] == 'h' || fName[strlen(fName) - 1] == 'H')){


       	   	printSearchCommand(grepFile , pattern);

       	   }

            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);

            listFilesRecursively(path,pattern);
        }
    }

    closedir(dir);
}

void searchCommand(char *args[]){


	int i=0; 
	while(args[i] != NULL){
		i++;
	}
	
	if(i==2){

		char cmd[1000];		

		// without -r option 
		// it will look all the files which ends .c .C .h .H under current directory and find the 'command' input word in this files


	    struct dirent *de;  // Pointer for directory entry 
	  
	    // opendir() returns a pointer of DIR type.  
	    DIR *dr = opendir("."); 
	  
	    if (dr == NULL){  // opendir returns NULL if couldn't open directory 

	        perror("Could not open current directory" ); 

	    } 
	  
	    /*
		*	Look all files under current directory and add find the files which endsWith .c .C .h .H , after finding call each of them and  
		*   look if the files includes the 'pattern' which comes with argument of search commend
	    */
	    while ((de = readdir(dr)) != NULL) {

	    	if(strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0){

				char fName[50];

				strcpy(fName,de->d_name);


	    		if(fName[strlen(fName) - 2] == '.' && (fName[strlen(fName) - 1] == 'c' || fName[strlen(fName) - 1] == 'C' ||
       	   		 fName[strlen(fName) - 1] == 'h' || fName[strlen(fName) - 1] == 'H')){

	    			printSearchCommand(de->d_name,args[1]);
	    		}

	    	}

	    }
	            
	  	printf("\n");
	    closedir(dr);     
 

	}
	else if(i==3 && strcmp(args[1],"-r")==0){

		// recursive code 


	    char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {

		    listFilesRecursively(cwd,args[2]);
		}
		else {

		    perror("getcwd() error");

		}

	}

	else{

		printf("2 ways to use this command :\nsearch 'command'\nsearch 'option' 'command'\n");

	}

}

void printUsageOfIO(){
	printf("[1] -> \"myprog [args] > file.out\"\n");
	printf("[2] -> \"myprog [args] >> file.out\"\n");
	printf("[3] -> \"myprog [args] < file.in\"\n");
	printf("[4] -> \"myprog [args] 2> file.out\"\n");
	printf("[5] -> \"myprog [args] < file.in > file.out\"\n");
}

//If input includes IO operation, then this method will return 0. Otherwise 1
//Contents of inputFileName and outputFileName also set in here 
int checkIORedirection(char *args[]){


	//Error handlings
	if(numOfArgs == 2 && strcmp(args[0], "io") == 0  && strcmp(args[1], "-h") == 0){
		printUsageOfIO();
		return 1;
	}

	int a;
	int io;
	for(a = 0; a < numOfArgs; a++){
		if(strcmp(args[a], "<") == 0 || strcmp(args[a], ">") == 0 || strcmp(args[a], ">>") == 0 || strcmp(args[a], "2>") == 0 ){
			io = 1;
		}
	}
	if(io == 1){
	}else{
		return 0;
	}

	int i;
	//Check arguments
	for(i = 0; i < numOfArgs; i++){
		if(strcmp(args[i], "<") == 0 && numOfArgs > 3  && strcmp(args[i+2],">") == 0){ // myprog [args] < file.in > file.out
			// We need to make sure there is a file_name entered too.
		    if(i+3 >= numOfArgs){
		        printf("Syntax error. You can type \"io -h\" to see the correct syntax.\n");
		        args[0] = NULL;
		        return 1;
		    }

		    inputRedirectFlag = 1;
		    outputRedirectFlag = 1;
		    strcpy(outputRedirectSymbol, args[i+2]);
		    strcpy(inputFileName , args[i+1]);
		    strcpy(outputFileName, args[i+3]);

		    return 0;

		}else if(strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], "2>") == 0 ){
			//We need to make sure there is a file_name entered too.
            if(i + 1 >= numOfArgs){
                printf("Syntax error. You can type \"io -h\" to see the correct syntax.\n");
                args[0] = NULL;
                return 1;
            }

            outputRedirectFlag = 1;
            strcpy(outputRedirectSymbol, args[i]);
            strcpy(outputFileName , args[i+1]);

            return 0;

		}else if(strcmp(args[i],"<") == 0){ // myprog [args] < file.in
			if(i + 1 >= numOfArgs){
                printf("Syntax error. You can type \"io -h\" to see the correct syntax.\n");
                args[0] = NULL;
                return 1;
            }

            inputRedirectFlag = 1;
            strcpy(inputFileName , args[i+1]);

            return 0;

		}

	}	


}


//This method clears the input from < > 2> >> . 
void formatInput(char *args[]){

	int i = 0;
	int a;
	int counter;
	int flag = 0;
	for(i = 0; i < numOfArgs; i++){
		if(strcmp(args[i],"<")== 0 || strcmp(args[i],">")== 0 || strcmp(args[i],">>")== 0 || strcmp(args[i],"2>")== 0){
			args[i] = NULL;
			a = i;
			counter = i+1;
			flag = 1;
			break;
		}
	}

	if(flag == 0) return; //Eger komutun io ile ilgisi yoksa direkt return

	for(i = counter; i < numOfArgs; i++){ 
		args[i] = NULL;
	}

	numOfArgs = numOfArgs - (numOfArgs-a); // Update number of arguments


}

 
int main(void){
	
	char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
	int background; /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2 + 1]; /*command line arguments */
	char path[PATH_MAX+1];
	char *progpath;
	char *exe;
	

	signal(SIGCHLD, childSignalHandler); // childSignalHandler will be invoked when the fork() method is invoked
	signal(SIGTSTP, sigtstpHandler); //This is for handling ^Z

	parentPid = getpid();
	
	ListProcessPtr startPtr = NULL; //starting pointer
	bookmarkPtr startPtrBookmark = NULL;

	while (parentPid==getpid()){
		background = 0;
		if(isEmpty(startPtr))		processNumber=1;
		printf("myshell: ");
		fflush(0);

		/*setup() calls exit() when Control-D is entered */
		setup(inputBuffer, args, &background);


		if(args[0] == NULL) continue; // If user just press "enter" , then continue without doing anything

		progpath = strdup(args[0]);
		exe=args[0];


		if(checkIORedirection(args) != 0){ //// Eger ıo yazımında vs hata varsa error verip yeniden input almalı			
			continue;
		}

		formatInput(args);

		if(strcmp(args[0],"exit")==0) {
			deleteStoppedList(&startPtr);
			if(isEmpty(startPtr) != 0){
				exit(1);
			}else{
				printf("There are processes running in the background!\n");				
			}
			
		}
		else if(strcmp(args[0],"ps_all")==0){
			printList(startPtr); //You need to press ps_all to see the content of list
			deleteStoppedList(&startPtr);
			
		}
		else if(strcmp(args[0],"search")==0){
			searchCommand(args);
			continue;
		}
		else if(strcmp(args[0],"bookmark")==0){

			bookmarkCommand(args,&startPtrBookmark);
			continue;
		}
		else if(!findpathof(path, exe)){ /*Checks the existence of program*/
			printf("No executable \"%s\" found\n", exe);
			free(progpath);
		}else{
					/*If there is a program, then run it*/
			if(*args[numOfArgs-1] == '&')// If last argument is &, delete it
				args[numOfArgs-1] = '\0';						
			createProcess(path,args,&background,&startPtr);
		}
		
		 	path[0] = '\0';	
			inputFileName[0] = '\0';
			outputFileName[0] = '\0';
			inputRedirectFlag = 0;
			outputRedirectFlag = 0;
	 }	  
        

				/** the steps are:
				(1) fork a child process using fork()
				(2) the child process will invoke execv()
				(3) if background == 0, the parent will wait,
				
				otherwise it will invoke the setup() function again. */
	
}
