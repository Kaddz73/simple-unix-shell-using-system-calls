# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# Name : Amal Kadri
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

# Dragonshell Readme

## Design Choices
### Step-by-Step Implementation
The implementation of `dragonshell` followed the requirements in a step-by-step manner to ensure correctness and completeness:
1. **Shell Loop**: The initial version of the shell consisted of a basic loop that prompts the user for input, tokenizes it, and executes the given command.
2. **Foreground and Background Process Handling**: Commands were implemented to either run in the foreground (waiting for completion) or in the background, allowing concurrent user interactions.
3. **Signal Handling**: `SIGINT` (`Ctrl+C`) and `SIGTSTP` (`Ctrl+Z`) were handled with `sigaction()` to ensure the shell properly manages interruptions and suspensions of foreground processes.
4. **Piping and Redirection**: Added functionality to handle command piping (`|`) and I/O redirection (`<`, `>`), providing a way to connect multiple commands and manage file input/output.
5. **Process Management**: Implemented tracking for processes, including background (`bg_pid`) and suspended processes (`s_pid`), to allow proper cleanup when the shell is closed.

### Object-Oriented Approach
The code is organized into distinct functions for each major task:
- **`new_process()`**: Handles the execution of commands, either in the foreground or background.
- **`signal_callback_handler()`**: Handles incoming signals to manage foreground process interruption or suspension.
- **`pipe_func()`**: Manages command piping for commands separated by the pipe symbol (`|`).

## System Calls Used
### `fork()`
- Used to create a child process whenever a new command is executed. It allows concurrent execution of the parent and child processes, enabling the shell to run and manage commands.
  
### `execve()`
- Used in child processes to replace the process image with a new program. This is used to execute the given command, making the child process run the desired executable.

### `waitpid()`
- Used to make the parent process wait for a child process to finish before proceeding. It is specifically used for managing both foreground and background processes (`WNOHANG` is used for non-blocking behavior).

### `kill()`
- Used to send specific signals to child processes. It is used to terminate, suspend, or resume processes, such as `SIGTERM` for termination or `SIGCONT` for resuming a suspended process.

### `dup2()`
- Used for I/O redirection in commands involving input (`<`) or output (`>`). It redirects the file descriptors to provide the desired file as input or output.

### `open()`
- Used for file handling in I/O redirection to open the files specified for reading (`<`) or writing (`>`). It provides the necessary file descriptors for `dup2()` to use.

### `close()`
- Used to close file descriptors after they are no longer needed, especially after redirection. This ensures that system resources are properly released and avoids file descriptor leaks.

### `sigaction()`
- Used to handle `SIGINT`, `SIGTSTP`, and `SIGCHLD`. This system call was chosen for more precise control over signal handling compared to `signal()`, allowing for custom behavior on receiving these signals.

### `times()`
- Used to measure the user and system time of the program.


## Testing
### Manual Testing
The implementation was tested step-by-step, in the same order as the features were developed, ensuring that each requirement was met before moving on to the next:

1. **Basic Shell Commands**: Commands like `ls`, `pwd`, and `cd` were executed to verify the basic shell loop functionality.
   - Example:
     ```sh
     pwd            # Displays the current working directory
     cd ..        # Changes the current directory to /tmp
     /usr/bin/ls             # Lists files in the current directory
     ```

2. **Foreground vs. Background Processes**: Commands were run with and without the `&` to test proper handling of foreground and background processes.
- I created two programs with an infinite while loop thats prints a message then sleeps for one second. I ran one in the background then executed the other. I also ran one then executed the other in the background.

3. **Signal Handling**:
   - **`Ctrl+C` (`SIGINT`)** was used to interrupt foreground commands and verify that only those commands were terminated.
- I ran a program with an infinite while loop that printed a message then sleeps for one second.

   - **`Ctrl+Z` (`SIGTSTP`)** was used to suspend processes, followed by resuming them to ensure the shell tracked suspended processes correctly.
        # Resumes the sleep process to run in the foreground
- I ran a program with an infinite while loop that printed a message then sleeps for one second.

4. **Piping and I/O Redirection**:
   - Commands using pipes (`|`) were tested to verify correct piping of output from one command to another.
     - Example:
       ```sh
       ls | grep filename   # Pipes the output of ls to grep to filter results
       ```
   - Commands with redirection (`>`, `<`) were tested to ensure files were correctly read or written as expected.
     - Example:
       ```sh
       echo "Hello World" > output.txt   # Redirects output to a file named output.txt
       cat < output.txt                  # Reads the contents of output.txt and prints it
       ```

5. **Exit Behavior**: The `exit` command was tested to ensure all running and suspended processes were properly terminated before the shell closed.
- After multiple program tests I ended each one with exit.

### Edge Cases
1. **Empty Input**: Tested to ensure the shell loop continued without error when no command was given.
   - Example:
     ```sh
     [Press Enter without typing anything]
     ```
2. **Multiple Background Processes**: Attempted to run multiple commands in the background to verify that only one was allowed, as intended.
   - Example:
     ```sh
     sleep 10 &       # Starts sleep in the background
     sleep 20 &       # Should display an error since only one background process is allowed
     ```
3. **Handling Suspended Processes**: Tested scenarios with suspended processes to ensure they were resumed and terminated correctly upon exiting.
   - Example:
     ```sh
     sleep 30         # Runs sleep for 30 seconds
     # Press Ctrl+Z to suspend sleep
     exit             # Ensure the suspended process is resumed and terminated
     ```


### Edge Cases
1. **Empty Input**: Tested to ensure the shell loop continued without error when no command was given.
2. **Multiple Background Processes**: Attempted to run multiple commands in the background to verify that only one was allowed, as intended.
3. **Handling Suspended Processes**: Tested scenarios with suspended processes to ensure they were resumed and terminated correctly upon exiting.

## Sources
- **Linux Programmer's Manual**: For understanding system calls like `fork()`, `execve()`, and `waitpid()`.
- **GNU `make` Documentation**: For creating and understanding the Makefile structure.
- **UNIX Network Programming, by W. Richard Stevens**: Provided insights into proper signal handling in concurrent processes.
- **Stack Overflow**: General reference for troubleshooting specific implementation issues with I/O redirection and signals.
- **YouTube Playlist**: [Operating System Development](https://www.youtube.com/playlist?list=PLfqABt5AS4FkW5mOn2Tn9ZZLLDwA3kZUY). This playlist was used to understand the basics of process management, system calls, and signals in Unix-based systems.
- **ChatGPT**: Used for assistance in creating the `readme.md` file and Makefile creation.
