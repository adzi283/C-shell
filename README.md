
# Personal Shell

## Project Overview

This project is a custom shell implementation in C. It is designed to interpret and execute user commands, manage job control, handle built-in commands, and provide a functional and user-friendly shell environment. The shell supports basic command execution, parsing, job control, and dynamic memory management, making it a robust tool for learning and experimenting with operating system concepts.

## Project Structure

The project is divided into several key components, each handled by corresponding C and header files:

### Command Parsing

- **Files**: `parser.c`, `argparse.c`, `parser.h`, `argparse.h`
- **Description**: 
    - **`parser.c`** handles breaking down the user input into commands and arguments. It uses tokenization logic to split the raw input into meaningful parts (commands and parameters) that can be executed. It ensures that the input is valid, and if necessary, prepares it for further processing.
    - **`argparse.c`** provides a utility for parsing command-line arguments. This module ensures that the input passed by the user is correctly interpreted, checks for argument syntax errors, and prepares it in the right format for execution by the shell.
- **Key Functionality**:
    - Breaking down user input into manageable tokens.
    - Ensuring proper argument parsing and validation.
    - Preparing the command and argument data structures.

### Shell Commands

- **Files**: `shellcmds.c`, `shellcmdutils.c`, `shellcmds.h`, `shellcmdutils.h`
- **Description**:
    - **`shellcmds.c`** implements the built-in shell commands such as `cd`, `exit`, and others. This file manages core functionalities that are typically expected from a shell. For instance, the `cd` command changes the current working directory, and `exit` allows the user to terminate the shell.
    - **`shellcmdutils.c`** provides supporting utilities for handling shell commands. These utilities include helper functions for command execution, path manipulation, and other related functionalities that the built-in commands rely on.
- **Key Functionality**:
    - Execution of built-in commands like `cd` and `exit`.
    - Utility functions for command handling and path manipulation.

### Job Control

- **Files**: `jobctrl.c`, `jobhandler.c`, `jobctrl.h`, `jobhandler.h`
- **Description**:
    - **`jobctrl.c`** manages job control, including handling foreground and background jobs. It allows the user to manage multiple processes simultaneously, enabling them to suspend or resume jobs as needed.
    - **`jobhandler.c`** provides additional support for job management by implementing features for tracking and managing job states, such as suspended, running, or terminated processes.
- **Key Functionality**:
    - Managing background and foreground processes.
    - Tracking and handling job state transitions (e.g., suspend, resume).

### Shell Environment

- **Files**: `shelldata.c`, `shelldata.h`
- **Description**:
    - **`shelldata.c`** is responsible for managing the shell's internal state, including environment variables and job lists. This file maintains the shell’s global settings, keeping track of user environment information, such as the current directory and active jobs.
- **Key Functionality**:
    - Maintaining and updating environment variables.
    - Managing the state of active jobs and processes.

### Utilities

- **Files**: `utils.c`, `mystring.c`, `vector.c`, `vecutils.c`, `utils.h`, `mystring.h`, `vector.h`, `vecutils.h`
- **Description**:
    - **`utils.c`** provides general-purpose utility functions that are used throughout the shell, such as error handling and memory management.
    - **`mystring.c`** contains custom string manipulation functions, such as trimming, tokenization, and comparison, designed to assist with parsing user input.
    - **`vector.c`** and **`vecutils.c`** implement dynamic arrays (vectors) for storing and managing lists of commands, arguments, and other shell data structures efficiently.
- **Key Functionality**:
    - Utility functions for memory management and error handling.
    - Custom string operations for input handling.
    - Dynamic arrays for flexible storage and retrieval of command-related data.

### System Wrappers

- **Files**: `wrappers.c`, `wrappers.h`
- **Description**:
    - **`wrappers.c`** provides wrapper functions around critical system calls, such as `fork`, `exec`, and `wait`. These wrappers simplify error handling and provide safer interfaces for executing system-level commands.
- **Key Functionality**:
    - Safer execution of system calls.
    - Error handling for system-level operations.

### Prompt Handling

- **Files**: `prompt.c`, `prompt.h`
- **Description**:
    - **`prompt.c`** manages the display of the shell prompt and interacts with the shell state to customize the prompt dynamically. It handles the user’s interface with the shell, ensuring the prompt reflects the current state (such as the working directory).
- **Key Functionality**:
    - Displaying a dynamic, user-friendly prompt.
    - Reflecting the shell’s current state in the prompt display.

## Files Overview

- **main.c**: The entry point of the shell. It initializes the shell environment, enters the main loop for processing user commands, and manages cleanup after the shell terminates.
- **parser.c**: Handles breaking down the user's input into commands and arguments, ensuring they are ready for execution.
- **shellcmds.c**: Implements built-in commands like `cd` and `exit` and handles their execution.
- **jobctrl.c**: Manages job control, including foreground and background processes, and handles job control signals.
- **utils.c**: Provides general utility functions for error handling, memory management, and other helper functions used throughout the shell.
- **vector.c**: Implements dynamic arrays (vectors) for managing lists of commands, arguments, etc.
- **wrappers.c**: Provides safer versions of system calls like `fork`, `exec`, and `wait`, with built-in error handling.
- **shelldata.c**: Manages the internal state of the shell, such as environment variables and job data.

## Compilation Instructions

This project uses a `Makefile` to automate the build process. Several make targets are provided for building, cleaning, and debugging the project.

### Default Build
- To build the project, run the following command:
  ```bash
  make build
  ```

### Debug Build
- To build the project with debugging information, run:
  ```bash
  make debug
  ```

### Clean
- To clean up the build artifacts, run:
  ```bash
  make clean
  ```

## Usage

1. **Starting the Shell**: After compiling, run the shell by executing:
   ```bash
   ./a.out
   ```
2. **Executing Commands**: You can execute built-in commands like `cd`, `exit`, or external commands like `ls`, `grep`, etc.
3. **Job Control**: Use job control commands like `fg` and `bg` to manage foreground and background jobs.

