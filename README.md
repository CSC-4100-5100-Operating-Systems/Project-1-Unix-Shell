# CSC 4100/5100 — Project 1: Unix Shell (“wish”)

A minimal Unix-like shell named **`wish`** that runs commands in interactive and batch modes, supports a small set of built-in commands, output redirection, and simple parallel execution.

> **Prompt:** `wish> ` (note the space after `>`)

> **Status:** Draft — some sections are placeholders and will be filled in soon.

---

## Project Description

Build a command-line interpreter that:
- Repeatedly prints a prompt, reads a command line, executes the command, and waits for it to finish.
- Runs in **interactive** mode (reads from stdin and prints the prompt) and **batch** mode (reads from a file and **does not** print the prompt).
- Spawns a child process for each external command (built-ins run in the shell process).


**Batch mode** reads commands from a file (no prompt). Hitting EOF exits cleanly.
1. takes in file to be processed and uses fopen(fileName) to open the file,
2. reads a line,
3. parses it into a command + args + flags(if any),
4. search for commands in the `/bin` and `/usr/bin` directories and if the command exists return the string `/bin` or `/usr/bin`,
5. appends the command name to the end of that string e.g. `/bin/pwd`,
6. implements `fork()` and in the chiuld process runs `execv()` and executes that command,
5. waits for the child to finish (`waitpid`) before continues on to the next line in the file or if it reaches EOF then exits gracefully.

> Note: if the command is a built in command, e.g. `cd` or `path`, the shell will handle these directly and not in a child process,

**Learning objectives**
- Get comfortable in the Linux/C environment  
- Learn how processes are created/managed (`fork`, `execv`, `wait`/`waitpid`)  
- Implement essential shell functionality

---

## Requirements
_Fill in soon._

---

## Usage

**Interactive Mode**
```bash
./wish
wish> pwd
wish> ls -l
wish> cd /tmp
wish> path /bin /usr/bin
wish> exit
```

**Batch Mode**

```bash
./wish fileName.txt
```
**Example Batch File in Folder called `batch_file.txt`**

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

## Build

Using the provided Makefile:
```makefile
make         # builds ./wish
make clean   # removes the binary
```

Or directly build with
```bash
gcc -o wish wish.c batch.c
```

---


### Implementation Notes
- Tokenization uses:
  
*Example from batch.c*
```c
// tokenize the line into lineArray[] using strsep
char *rest = line;
char *lineArray[16];
int index = 0;
char *strsepToken;

// strsep reference: https://c-for-dummies.com/blog/?p=1769
while ((strsepToken = strsep(&rest, " \t\n")) != NULL) {
    if (*strsepToken == '\0') continue;          // skip empty tokens
    if (index == 15) break;          // leave room for NULL
    lineArray[index++] = strsepToken;
}
```

---

### Contributors

- Brian Kemp — bkemp42@tntech.edu
- Lewis Bates — lfbates42@tntech.edu
- Oluwadara Odukoya — ojodukoya42@tntech.edu
- Meicheng Xiao — mxiao42@tntech.edu
