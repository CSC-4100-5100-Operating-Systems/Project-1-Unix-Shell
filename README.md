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

**Learning objectives**
- Get comfortable in the Linux/C environment  
- Learn how processes are created/managed (`fork`, `execv`, `wait`/`waitpid`)  
- Implement essential shell functionality

---

## Requirements
_Fill in soon._

---

## Usage

### Build
_Fill in soon._

```bash
# Example (if using Makefile)
make
```

---

### Contributors

- Brian Kemp — bkemp42@tntech.edu
- Lewis Bates — lfbates42@tntech.edu
- Oluwadara Odukoya — ojodukoya42@tntech.edu
- Meicheng Xiao — mxiao42@tntech.edu
