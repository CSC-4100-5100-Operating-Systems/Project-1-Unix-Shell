# CSC 4100/5100 — Project 1: Unix Shell (“wish”)

A minimal Unix-like shell named **`wish`** that supports:
- **Interactive** and **batch** modes
- **Built-ins**: `cd`, `path`, `exit`
- **External commands** via `fork()` + `execv()` + `waitpid()`
- **Parallel execution** with `&` on a single line
- **Output redirection** with `>`
- A dynamic prompt `user@host:cwd$ wish> `
- Line editing, history, and **Tab filename completion** (via vendored **linenoise**)

---

## Project Layout

```text
.
├── batch.c            # Batch mode: read file, parse lines, run (serial)
├── batch.h
├── commands.c         # Interactive mode executor + built-ins (cd/path/exit)
├── commands.h
├── parallel.c         # Split on '&', run jobs in parallel, optional '>'
├── parallel.h
├── wish.c             # Entry point, interactive loop (linenoise), batch switch
├── wishCwdPrompt.c    # Builds dynamic prompt (user@host:~/path$ wish> )
├── wishCwdPrompt.h
├── linenoise.c        # Vendored tiny line editor (BSD-like license)
├── linenoise.h
├── makefile
├── batch_file.txt     # Example batch file (sample commands)
└── README.md
```

---

## Build

Using the provided Makefile:
```makefile
make         # builds ./wish
make clean   # removes the binary
```

Or directly build with
```bash
gcc -o wish wish.c batch.c parallel.c commands.c wishCwdPrompt.c linenoise.c
```

---

## Project Description

Build a small Unix-like shell called **`wish`**. The shell supports:

- **Interactive mode**: prints a prompt, reads a line, executes the command, repeats.
- **Batch mode**: reads commands from a file (no prompt) and executes them sequentially.
- **Built-ins**: `cd`, `path`, `exit` (handled in the shell process).
- **External commands**: located via the current search path and executed with `fork()` + `execv()` + `waitpid()`.
- **Parallel execution**: multiple commands on one line separated by `&`, executed concurrently.
- **Output redirection**: `>` redirects both stdout and stderr to a file.
- **Usability**: dynamic prompt `user@host:cwd$ wish> `, line editing, history, and Tab completion (via vendored `linenoise`).

### How interactive mode works
1. Show the prompt `user@host:cwd$ wish> `.
2. Read a line, keep it in history, and tokenize it into `argv`.
3. If it’s a **built-in**:
   - `exit`: no arguments; terminates the shell with status 0.
   - `cd <dir>`: exactly one argument; changes the shell’s current directory.
   - `path [dir1 dir2 ...]`: overwrites the shell’s executable search path. If empty, only built-ins work.
4. Otherwise, resolve `argv[0]` against the current path (default includes `/bin` and `/usr/bin`), then:
   - `fork()` a child, `execv(full_path, argv)` in the child, and `waitpid()` in the parent.
5. If the line contains `&`, split it into jobs, spawn one child per job (with optional `>` redirection per job), and wait for all children to finish.

### How batch mode works
Batch mode reads commands from a file (no prompt). On EOF, the shell exits cleanly.

1. `fopen(batch_file)`.
2. `getline()` to read each line.
3. Tokenize into command + args (flags included, e.g., `ls -l`).
4. Built-ins handled in-process (`cd`, `path`, `exit`); external commands are resolved via the path list and executed with `fork()`/`execv()`.
5. Wait (`waitpid()`) for the command to finish, then read the next line until EOF.

> **Notes**
> - The project uses `execv()` (not `system()`).
> - Redirection syntax supported: `cmd args > file` (one `>` per job; truncates/creates).
> - Built-ins inside a parallel line are rejected (prints the spec’s single error message).
> - The prompt collapses `$HOME` to `~` and updates after `cd`.

### Learning objectives
- Comfort with the Linux/C toolchain and system calls.
- Process creation and management with `fork()`, `execv()`, and `wait`/`waitpid()`.
- Implementing a simple command parser and search path logic.
- Understanding interactive vs. batch execution, redirection, and basic concurrency.

---

## Requirements

### Executable & Invocation
- The program must build to an executable named **`wish`**.
- Valid invocations:
  - **Interactive:** `./wish`
  - **Batch:** `./wish <batch_file>`
- Anything other than zero or one argument is an error; the shell must print the single error message and **exit(1)**.

### Modes
- **Interactive mode**
  - Display a prompt: `wish> ` (our implementation shows `user@host:cwd$ wish> ` for readability).
  - Read, parse, execute one line at a time, then repeat until `exit`.
- **Batch mode**
  - **No prompt** is printed.
  - Read and execute one line at a time from the given file.
  - On **EOF**, exit cleanly with **exit(0)**.

### Input & Parsing
- Use a line-based reader (e.g., `getline()` or equivalent) to accept arbitrarily long lines.
- Tokenize input using whitespace (spaces/tabs/newlines). `strsep()` is recommended.
- Whitespace around commands, arguments, and operators should be tolerated.
- Operators do **not** require whitespace (e.g., `echo hi>out.txt` is acceptable).

### Built-in Commands (handled in the shell process)
- **`exit`**
  - Usage: `exit`
  - **No arguments** allowed; otherwise print error.
  - On valid usage, terminate the shell with **exit(0)**.
- **`cd`**
  - Usage: `cd <dir>`
  - **Exactly one** argument required; otherwise print error.
  - Use `chdir()`; if it fails, print error.
- **`path`**
  - Usage: `path [dir1 dir2 ...]`
  - Overwrites the current search path with the provided directories.
  - If no args are given, the path becomes empty → only built-ins can run.
  - Initial path must contain **`/bin`** (our interactive path sets `/bin` and `/usr/bin` by default; batch path honors the spec and can be modified by `path`).

### External Commands
- For non–built-in commands, create a child process to execute the program.
- Use **`fork()`** in the parent; in the **child**, call **`execv()`**.
  - **Do not** use `system()`.
  - If `execv()` succeeds it will not return; if it returns, treat as an error.
- Wait for each child with **`wait()`/`waitpid()`** (interactive: serial; parallel: wait for all on the line).

### Path Resolution
- Resolve `argv[0]` by searching, **in order**, the directories listed by `path`.
- Use `access(candidate, X_OK)` to test executability (e.g., `/bin/ls`, then `/usr/bin/ls`, etc.).
- If no candidate exists/is executable, print the single error message.

### Output Redirection
- Syntax: `command args > filename`
  - Redirect **both stdout and stderr** to `filename`.
  - Create the file if it does not exist; truncate if it does.
- Exactly **one** `>` per job and **exactly one** filename to the right.
- Redirection of built-ins is **not required** (inputs will not test it).
- Malformed redirection (multiple `>` or multiple filenames) is an error.

### Parallel Commands
- A single input line may contain multiple commands separated by `&`.
  - Example: `cmd1 & cmd2 args & cmd3`
- Launch each command in **parallel** (one child per job).
- After starting all jobs on the line, **wait for all** to complete before continuing.
- Built-ins within a parallel line are **not allowed** (treat as error).

### Errors
- On **any** error condition, print exactly the following to **stderr**: `An error has occurred`
- After most errors, continue processing the next command/line.
- On shell invocation with **>1 arguments** or a **bad batch file**, print the error and **exit(1)**.

### End-of-File (EOF)
- In interactive or batch mode, on EOF, the shell should **exit(0)** gracefully.

### Additional Notes / Assumptions
- Absolute and relative explicit paths (e.g., `/bin/ls`, `./a.out`) are not required by the spec; the shell resolves via `path`.
- The prompt string is not graded; the required behavior is only that a prompt is printed in interactive mode and **not** printed in batch mode.
- The implementation uses `linenoise` for line editing/history/completion in interactive mode; this is internal to our shell and does not change required behavior.

---

## Usage

**Interactive Mode**
```bash
./wish
user@host:~/Desktop/Project_1$ wish> pwd
user@host:~/Desktop/Project_1$ wish> ls -l
user@host:~/Desktop/Project_1$ wish> cd test
user@host:~/Desktop/Project_1/test$ wish> path /bin /usr/bin
user@host:~/Desktop/Project_1/test$ wish> exit
```
### Notes

- **Tab completion:** Completes the last token against files and directories in the current working directory.  
- **History:** Use ↑ / ↓ to recall previous commands (provided by `linenoise`).

### **Batch Mode**

```bash
./wish fileName.txt
```
 - Opens the file.  
 - Reads one line at a time.  
 - Parses into **command + arguments** (tokens split on spaces/tabs).  
 - Resolves the command by searching the configured path directories (default `/bin:/usr/bin`).  
 - Runs with `fork()` + `execv()`.  
 - Waits (`waitpid()`) before proceeding to the next line.  

 **Built-ins** (`cd`, `path`, `exit`) are executed within the shell process.

### **Example Batch File in Folder called `batch_file.txt`**

```bash
pwd
cd /tmp
pwd
mkdir wish_test
cd wish_test
pwd
touch a.txt
touch b.txt
ls -l
chmod 600 a.txt
stat a.txt
mv b.txt c.txt
cp a.txt copy_a.txt
ls -l
whoami
id
uname -s
date
ls -la
cd ..
ls -1
rm -rf wish_test
pwd
```

---

## Features

### Built-ins (Interactive + Batch)

- #### `exit`
  - **Usage:**  `exit`
  - No arguments allowed. Exits the shell with status `0`.
- #### `cd`
  - **Usage:**  `cd <dir>`
  - Exactly one argument required. Changes the shell’s current directory.
- #### `path`
  - **Usage:**  `path [dir1 dir2 ...]`
  - Replaces the shell’s search path with the provided list.
  - If no args are given, the path becomes empty → only built-ins will work.
  - Defaults include /bin and /usr/bin on startup (interactive uses its own list; batch maintains an internal list as well).

### External Programs
- The shell searches each directory in the current path for an executable named `argv[0]`,  
  using `access(X_OK)`, then runs it with:
  - `fork()` → child process
  - `execv(full_path, argv)` → replace child
  - `waitpid()` in the parent

### Parallel Execution (`&`)
- A single input line can contain multiple commands separated by &.
- Example:
    ```bash
    pwd & whoami & uname -s & date
    ```
- Each chunk becomes a separate job. The shell forks each command, then waits for all to finish.

> **Note:** Built-ins are **not allowed** inside a parallel line (per project simplification).  
> Attempting to use `cd`, `path`, or `exit` within a parallel group yields the standard error message.

### Output Redirection (`>`)
- **Form:** `cmd args > filename`
- Redirects both **stdout** and **stderr** of the job to `filename` (truncates or creates).  
- Only **one** `>` is supported per job.  
- Multiple or malformed redirections print the error message: `An error has occurred`

### Prompt
- **Dynamic format:** `user@host:cwd$ wish>`
- Collapses `$HOME` prefix to `~`.

### Tab Completion & History

Provided by **linenoise** (vendored).
- **Completion:** Filename completion of the last token in the current directory.  
- **History:** Use arrow keys (↑/↓); persisted in memory for the session.
  - In interactive mode, history is added using `linenoiseHistoryAdd(rl);` **before tokenization** to preserve the original command string (since `strsep` mutates the buffer).

---

## Examples

### Simple commands
```bash
wish> pwd
wish> ls -la
wish> whoami
wish> date
```

### Simple commands
```bash
wish> cd /tmp
wish> path /bin /usr/bin /usr/local/bin
wish> exit
```

### Redirection
```bash
wish> ls -1 > out.txt
wish> uname -a > sysinfo.txt
```

### Parallel lines
```bash
wish> pwd & whoami & uname -s & date
wish> echo A & sleep 1 & echo B & sleep 2 & echo C
wish> ls -1 & sleep 1 & echo "done listing"
```

---

## Implementation Notes

### Tokenization

 - Uses `strsep()` to split on spaces, tabs, or newlines. Empty tokens are skipped.

 - **Example (from `batch.c`):**
    ```c
    char *rest = line;
    char *lineArray[16];
    int index = 0;
    char *tok;
    while ((tok = strsep(&rest, " \t\n")) != NULL) {
        if (*tok == '\0') continue;
        if (index == 15) break;
        lineArray[index++] = tok;
    }
    lineArray[index] = NULL;

### Path Search

Interactive and batch modes each maintain a simple in-memory list (`directories[]` or `search_paths[]`) of up to **64 entries**.

- Default includes `/bin` and `/usr/bin`.  
- The `path` command overwrites the list.

### Processes

- **External commands:** `fork()` → `execv()` → parent `waitpid()`.  
- **Parallel commands:** one child per job; parent waits for all PIDs spawned from the line.

### Errors

Per spec, all runtime errors print the single message: `An error has occurred`

---

## Development & Notes

**Dependencies:**  
No external system libraries required. `linenoise` is vendored (`linenoise.c` / `linenoise.h`) under a BSD-like license and compiled into the binary.

**Editor/IDE:**  
If using VS Code, you may ignore local workspace files by adding to `.gitignore`:

```gitignore
.vscode/*
!.vscode/settings.json
!.vscode/extensions.json
```

**Platform:**  
Tested on Ubuntu-like Linux (paths `/bin`, `/usr/bin`).

---

### Contributors

- Brian Kemp — bkemp42@tntech.edu
- Lewis Bates — lfbates42@tntech.edu
- Oluwadara Odukoya — ojodukoya42@tntech.edu
- Meicheng Xiao — mxiao42@tntech.edu

---

## Acknowledgments

**linenoise** by *Salvatore Sanfilippo* and *Pieter Noordhuis* (BSD-like license).  
Vendored [here](https://github.com/antirez/linenoise) to provide line editing, history, and completion in a tiny, portable way.
