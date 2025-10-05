# Yash Shell

Yash is a simple interactive shell program implemented in C. It serves as a command interpreter, providing a user interface to the operating system for executing commands. Yash supports interactive command execution, built-in commands, pipe operators, and basic process management, while ignoring SIGINT signals (e.g., Ctrl-C) to ensure the shell remains responsive.

## Features

- **Command Execution**: Accepts user commands and executes corresponding programs with their argument lists.
- **Path Resolution**: Locates and runs executable programs via:
  - Absolute paths (starting with `/`).
  - Relative paths (starting with `./`).
  - Searching directories in the `$PATH` environment variable.
- **Built-in Commands**:
  - `exit`: Terminates the Yash shell. The shell runs continuously until this command is issued.
  - `watch`: Monitors the resource usage of a running child process until it terminates.
- **Pipe Support (`|`)**: Chains multiple commands together, allowing output from one command to serve as input for the next.
- **Signal Handling**: The Yash process ignores SIGINT (Ctrl-C), preventing accidental termination.
- **Input Limits**: Supports command lines up to 1024 characters, with up to 30 tokens (including `|` symbols).

## Behavior

- **Prompt**: When ready for input, Yash displays:
  ```
  yash >>
  ```
- **Parsing and Execution**:
  - Reads a line of input from the user.
  - Parses the line into command name(s) and argument list(s), handling pipes as needed.
  - Forks child process(es) to execute the command(s).
  - Waits for all child processes to terminate before displaying the next prompt.
- **Assumptions**:
  - Commands are executed in a Unix-like environment.
  - Only compiled (executable) programs are supported.

## Building and Installation

### Prerequisites
- A C compiler (e.g., `gcc`).
- Unix-like system (tested on Linux/macOS).

### Compilation
1. Clone or download the source code.
2. Compile the program:
   ```bash
   gcc -o yash yash.c
   ```
   (Replace `yash.c` with the actual source file name if different.)

3. Run the shell:
   ```bash
   ./yash
   ```

No additional installation is required; Yash runs as a standalone executable.

## Usage

### Basic Commands
Enter commands at the `yash >>` prompt. Examples:
```
yash >> ls -la
yash >> /bin/echo "Hello, World!"
yash >> ./myprogram arg1 arg2
```

### Built-in Commands
- Exit the shell:
  ```
  yash >> exit
  ```
- Monitor a process:
  ```
  yash >> watch ls -la
  ```
  This will run `ls -la` and display resource usage until completion.

### Pipes
Chain commands with `|`:
```
yash >> ls | grep .txt
yash >> cat file.txt | wc -l
```
Yash parses pipes and sets up the necessary inter-process communication.

### Ending a Session
Type `exit` to quit. Yash will not terminate on Ctrl-C.

## Limitations
- No support for redirection (`>`, `<`), environment variable expansion, or scripting.
- Maximum 30 tokens per command line.
- Assumes all executables are binary programs; no shell scripts.
- Resource monitoring via `watch` is basic and focuses on child process stats.

## Contributing
Feel free to fork the repository, submit issues, or send pull requests for enhancements like additional built-ins or error handling.

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
```
