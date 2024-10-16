#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <stdint.h>
#include <sys/times.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <signal.h>

#define LINE_LENGTH 100 // Max # of characters in an input line

pid_t bg_pid = -1; //global variable to track bg process
pid_t s_pid = -1; //global variable to track one suspened process
pid_t child_pid = -1;
void signal_callback_handler(int signum) {
  printf("\n");

  if (child_pid > 0) { 
    if(signum == SIGTSTP && s_pid == -1 ){ // only suspending one process
    s_pid = child_pid;
  }
    kill(child_pid, signum); // Forward the signal to the child process
    child_pid = -1;
  }

}

void tokenize(char* str, const char* delim, char ** argv){
  char* token;
  token = strtok(str, delim);
  size_t i = 0;
  while(token != NULL){
    argv[i] = token;
    token = strtok(NULL, delim);
    ++i;
  }
  argv[i]=NULL; //ensure array ends with NULL
}

void cd_func(char* next_path){
  next_path[strcspn(next_path, "\n")] = '\0';  // remove newline from fgets and replacing with NULL
  if (chdir(next_path) == -1) {
        printf("dragonshell: No such file or directory\n");
    }
	return;
}

void pwd_func(){
    char s[100]; // s will hold the path of current directory
    printf("%s\n", getcwd(s, 100));
  return;
}

void new_process(char **args, bool bg){
  /*This function takes in args: the commands, and bg: whether or not to run a process in the background.
   *This functions deals with all external commands but piping */

  //check if there is already a background process running
  if (bg && bg_pid != -1) {
    printf("dragonshell: Only one background process is allowed at a time.\n");
    return;
  }

  //creating a child process
  pid_t pid = fork(); 
  char *newenviron[] = { NULL }; //3rd parameter of execve
  if (pid==0){ //we are in the child process  
    if (bg) {
      // Set up sigaction to ignore SIGINT and SIGTSTP for background processes
      struct sigaction sa_ignore;
      sa_ignore.sa_handler = SIG_IGN; // Set the handler to SIG_IGN
      sigemptyset(&sa_ignore.sa_mask);
      sa_ignore.sa_flags = 0;

      // Ignore SIGINT
      if (sigaction(SIGINT, &sa_ignore, NULL) == -1) {
        perror("sigaction failed for SIGINT");
        _exit(EXIT_FAILURE);
      }

      // Ignore SIGTSTP
      if (sigaction(SIGTSTP, &sa_ignore, NULL) == -1) {
        perror("sigaction failed for SIGTSTP");
        _exit(EXIT_FAILURE);
      }
    }
    
    //checking if the command requires I/O redirection
    for (int i = 0; args[i] != NULL; i++) {//looping through args array to find '<' or '>' for I/O redirection

      if (strcmp(args[i], ">") == 0) { // output redirection
        int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644); //write/creat/truncate
    
        if (fd == -1) { //opening file fails
          perror("open failed for output redirection");
          _exit(0);}

        //redirectin output to file descriptor
        if (dup2(fd, STDOUT_FILENO) == -1) {
          perror("dup2 failed for output redirection");
          _exit(0);}
                
        close(fd); //close file
        args[i] = NULL; //remove redirection part from args

      }else if (strcmp(args[i], "<") == 0) { //input redirection
        int fd = open(args[i+1], O_RDONLY); //read only

        if (fd == -1) { //opening file fails
          perror("open failed for input redirection");
          _exit(0);}

        //redirection input to file descriptor  
        if (dup2(fd, STDIN_FILENO) == -1) {
          perror("dup2 failed for input redirection");
          _exit(0);}
                
        close(fd);
        args[i] = NULL; //remove redirection part from args
      } 
    }
  
    execve(args[0],args,newenviron); //executing the command
    printf("dragonshell: Command not found\n"); //execve returns upon failure print appropriate command
    _exit(0);
  } else if (pid>0){ //we are in the parent process
    if(bg){ //running a program in the background
      printf("PID %d is sent to background\n", pid);
      bg_pid = pid; //storing the background pid
      
    }else{  //foreground process
      child_pid = pid;
      int status;
      waitpid(pid, &status, 0);  //wait for process to finish
      child_pid = -1;  
  }
    
  }else{
    perror("fork failed!");}
       
  return;
}

void pipe_func(char** commands, int pipe_index){

  //creating commands as char ** to operate in execve
  char ** cmd1= commands; // array has cmd1 then arguments followed by NULL
  char ** cmd2= &commands[pipe_index +1]; // array has cmd2 then arguments followed by NULL
  //creating pipe
  int fd[2];
  if(pipe(fd) == -1){
    perror("pipe failed");
    _exit(0);
  } 

  pid_t pid1 =fork();
  if(pid1 == 0){
    //1st child proccess to write cmd1 into pipe
    close(fd[0]); //close unused read end of pipe
    //redirecting output to write end of pipe
    if (dup2(fd[1], STDOUT_FILENO) == -1) {
      perror("dup2 failed for cmd1"); //write fails
      _exit(0);
    }
    close(fd[1]); // close write end of pipe  
    //execute cmd1
    char *newenviron[] = { NULL };
    execve(cmd1[0], cmd1, newenviron);
    perror("exec failed for cmd1"); //exec fails
    _exit(0);
  }

  pid_t pid2 = fork();
  if (pid2 == 0) {
    //2nd child proccess to read and execute cmd2 into pipe
    close(fd[1]); // close unused write end of pipe
    //redirectin input to read end of pipe
    if (dup2(fd[0], STDIN_FILENO) == -1) {
      perror("dup2 failed for cmd2");
      _exit(0);
    }
    close(fd[0]); // close read end of pipe
    // execute cmd2
    char *newenviron[] = { NULL };
    execve(cmd2[0], cmd2, newenviron);
    perror("exec failed for cmd2");
    _exit(0);
  }

  // closing both ends of pipe
  close(fd[0]);
  close(fd[1]);
  //waiting for children (free to assume no signal will be sent while pipe. *breaks one process in background rule*)
  wait(NULL);
  wait(NULL);
  return;
}
void setup_signals(){
  //setting up signal handling
  struct sigaction sa;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = signal_callback_handler;

  //handler for ctrl-c
  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction failed for SIGINT");
    _exit(0);
  }
  //handler for ctrl-z
  if (sigaction(SIGTSTP, &sa, NULL) == -1) {
    perror("sigaction failed for SIGTSTP");
    _exit(0);
  }
  return;
}

int main(){
  setup_signals(); //setting up signal handling

  //setting up begginning of timer
  struct tms start_buf, end_buf;
  long ticks_per_second = sysconf(_SC_CLK_TCK);
  times(&start_buf);
	
  char *input = calloc(LINE_LENGTH,sizeof(char));  // Allocate 100 characters for input lines
  char **array = calloc(25,sizeof(char*)); // Allocate array for 25 string pointers

  printf("Welcome to Dragon Shell!\n"); // welcome text

  while(1){ // shell loop
    //clearing input and buffer
    for (int i = 0; i < 25; i++) {
      array[i] = NULL; }

    memset(input, '\0',LINE_LENGTH); 

    //checking if background process is still running to ensure only 1 runs at a time
    if (bg_pid > 0) {
      int status;
      pid_t result = waitpid(bg_pid, &status, WNOHANG); //using WNOHANG to let shell keep running while checking
      if (result > 0) {
          bg_pid = -1; //reset bg_pid since background process has ended
      }
    }

    //getting input
    printf("dragonshell > ");
    fgets(input, LINE_LENGTH, stdin);

    input[strcspn(input, "\n")] = '\0';
    
    tokenize(input," ", array);
    fflush(stdin);

    //dealing with array when given no input
    if(array[0] == NULL){ 
      array[0]="empty";
    }

    //input handling
    if(strcmp(array[0], "empty") == 0){
      continue; //no input given

    }else if(strcmp(array[0], "cd") == 0){
      if (array[1] == NULL ){ //no argument was given with "cd"
        printf("dragonshell: Expected argument to \"cd\"\n"); 

      }else if (array[1] !=NULL){
        cd_func(array[1]); } //send path to cd function   
        
    }else if(strcmp(array[0], "pwd") == 0){
      pwd_func();

    }else if(strcmp(array[0], "exit") == 0){
      //terminate any process running
      if (s_pid > 0) {
        kill(s_pid, SIGTERM);
        waitpid(s_pid, NULL, 0);  // Ensure the suspended process is terminated
      }
      if (child_pid > 0) { 
        kill(child_pid, SIGTERM);
      }
      if (bg_pid > 0) {
        // Restore default signal handling for SIGINT and SIGTSTP
        struct sigaction sa_default;
        sa_default.sa_handler = SIG_DFL; // Set handler to default
        sigemptyset(&sa_default.sa_mask);
        sa_default.sa_flags = 0;
        // without reseting ther may be trouble sending anykind of SIG like SGITERM
        if (sigaction(SIGINT, &sa_default, NULL) == -1) {
            perror("sigaction failed for SIGINT");
        }

        if (sigaction(SIGTSTP, &sa_default, NULL) == -1) {
            perror("sigaction failed for SIGTSTP");
        }

        kill(bg_pid, SIGTERM);
        waitpid(bg_pid, NULL, 0);  //waiting for background process to terminate
        bg_pid = -1; // Reset bg_pid after termination
      }
      
      break; //exit the while loop

    }else{
      //dealing with external commands
      int i =0, pipe_index =0;
      bool cmd = false; //if piping is required
      bool bg = false; //if background proccessing is required
      while(array[i] != NULL){
        //checking for | to pipe or & to background a process
        if(strcmp(array[i], "|") == 0){
          cmd = true;
          array[i]=NULL;
          pipe_index =i;
          
        }else if(strcmp(array[i], "&") == 0){
          bg =true;
          array[i]=NULL;
        }
        i++;
      }

      if(cmd == true){
        pipe_func(array, pipe_index); //function pipe take in arguments and index of |

      }else{
        new_process(array, bg); //function take in arguments and bg bool if necassary
      }  
    }

    //clearing input and buffer
    for (int i = 0; i < 25; i++) {
      array[i] = NULL; }

    memset(input, '\0',LINE_LENGTH); 
  }

  //free memory
  free(input);
  free(array);
  //getting/ outputting time
  times(&end_buf);
  printf("User time: %lf\n", (double)(end_buf.tms_utime - start_buf.tms_utime) / ticks_per_second);
	printf("Sys time: %lf\n", (double)(end_buf.tms_stime - start_buf.tms_stime) / ticks_per_second);
  
return 0; //return will exit
}