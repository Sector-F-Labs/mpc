# Man Page Compiler (MPC)

Turn a Unix man page (`.1`) into a real, working CLI program. You write the documentation first; MPC makes the implementation match it.

## What is this?

MPC is a **documentation-as-source compiler**: the man page becomes the canonical specification, and the code is generated to fulfill it.

## Philosophy: Inverting the Usual Direction

Traditionally: write code → (maybe) write docs → docs drift.
With MPC: write precise docs → get matching code → docs stay the source of truth.

Man pages already define: interface, behavior, options, examples, exit codes. MPC treats that as an executable spec and generates C code that conforms to it.

## Example

Input file (`calculator.1`):
```roff
.TH CALCULATOR 1
.SH NAME
calculator \- add two integers and output the sum
.SH SYNOPSIS
.B calculator
.I number1
.I number2
.SH DESCRIPTION
The
.B calculator
utility takes two integer arguments and outputs their sum to stdout.
.SH OPTIONS
.TP
.BR \-h ", " \-\-help
Display help information and exit.
.SH EXAMPLES
$ calculator 42 13
55
```

Output: A working `calculator` binary that actually behaves exactly as documented.

## Quick Start

```bash
./configure                    # Auto setup everything
./bin/mpc examples/simple.1    # Generate program
./simple                       # Run it
```

That's it! The `./configure` script automatically installs dependencies, sets up Ollama, builds MPC, and tests everything.

## Setup

```bash
./configure
```

That single command installs what’s needed, builds MPC, fetches a default local model (codellama:7b via Ollama), and runs a sanity check.

Remote instead of local?  
```bash
export GEMINI_API_KEY=your_key
mpc --remote examples/simple.1
```

See SETUP.md for optional manual / platform notes (kept out of this README to stay lean).

## Usage

```bash
mpc examples/simple.1    # Generate from man page
./simple --help          # Run the result
```

Run `mpc --help` for options.

## How It Works

```
Man Page → Parse → LLM → Generated Code → Executable
```

When compilation fails, MPC feeds error messages back to the LLM for automatic fixes.

## Mailing List Hypothesis

LLMs are unusually strong at generating C because decades of public systems programming mailing lists (kernel, BSDs, GNU, POSIX) combine code, rationale, review, corrections, portability notes, and security discussion in one stream. That mixed signal (not just code blobs, but argued code) trains models in patterns + intent. MPC leverages this: a well-structured man page mirrors the specification style in those archives, so the model reliably maps the documented interface to working, idiomatic C.

### Why it matters here

Instead of listing ecosystems and repeating evidence, the key takeaway is: documentation written in the terse, declarative, interface-first style (like man pages) aligns with the discourse form the model learned from—so generation quality is higher and MPC can stay minimal.

**The Big C Ecosystems:**
- **Linux Kernel** - LKML (Linux Kernel Mailing List) with millions of messages
- **FreeBSD** - Technical mailing lists with detailed code discussions
- **OpenBSD** - Security-focused development discussions
- **NetBSD** - Portability and architecture discussions
- **GNU Project** - Coreutils, GCC, and system tool development
- **POSIX Standards** - IEEE specifications with extensive examples

**Why Mailing Lists are LLM Gold:**
- **Code + Context**: Every patch comes with detailed explanations
- **Error Corrections**: Mistakes are publicly discussed and fixed
- **Best Practices**: Senior developers explain *why* code should be written certain ways
- **Real Problems**: Actual bugs, edge cases, and production issues
- **Iterative Refinement**: Multiple revisions with explanations for each change

Net effect: the model doesn’t just emit C syntax—it reproduces the reasoning patterns behind small, composable Unix-style tools. That is exactly what MPC turns into an executable.



**The Hypothesis:** Languages with rich mailing list cultures produce better LLM training data because they include the *reasoning* behind code decisions, not just the code itself. The verbose, explanatory nature of email discussions creates ideal training material for understanding programming intent and best practices.

**Why C Projects Excel at This:**
- **Backwards compatibility** requirements force detailed explanations
- **Performance implications** require justification for every decision
- **Security considerations** are explicitly discussed
- **Portability concerns** are thoroughly analyzed
- **Maintenance burden** is a constant topic of discussion

This creates training data that teaches LLMs not just C syntax, but C *thinking*.



## Examples

Try the included examples:
```bash
mpc examples/hello.1      # Simple greeting program
mpc examples/count.1      # Line counter
mpc examples/calculator.1 # Add two numbers
```

See `examples/` directory for more. Details in `man mpc`.













## License

BSD 3-Clause License.











---

**MPC: Documentation is the source; code is the build artifact.**

*Write the man page. Run the result.*
