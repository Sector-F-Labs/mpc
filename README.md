# Man Page Compiler (MPC)

Turn a Unix man page (`.1`) into a real, working CLI program. You write the documentation first; MPC generates the implementation to match it.

---

## What is this?

MPC is a **documentation-as-source compiler**: the man page is treated as the canonical specification, and the generated code is built to conform to it.

---

## Philosophy: Inverting the Usual Direction

Traditional workflow: write code → (maybe) write docs → docs drift out of sync.

MPC workflow: write precise docs → generate matching code → docs remain the source of truth.

Man pages already define interface, behavior, options, examples, and exit codes. MPC treats this as an *executable specification* and produces C programs that implement it.

---

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

Output: a working `calculator` binary that behaves exactly as documented.

---

## Quick Start

```bash
./configure                    # Auto-setup everything
./bin/mpc examples/simple.1    # Generate program
./simple                       # Run it
```

The `./configure` script automatically installs dependencies, sets up Ollama, builds MPC, and runs a quick test.

---

## Setup

```bash
./configure
```

That single command:

* Installs prerequisites
* Builds MPC
* Fetches a default local model (`codellama:7b` via Ollama)
* Runs a sanity check

Remote instead of local?

```bash
export GEMINI_API_KEY=your_key
mpc --remote examples/simple.1
```

See `SETUP.md` for optional platform-specific notes.

---

## Usage

```bash
mpc examples/simple.1    # Generate from man page
./simple --help          # Run the result
```

Run `mpc --help` for available options.

---

## How It Works

```
Man Page → Parse → LLM → Generated Code → Executable
```

When compilation fails, MPC captures compiler errors and feeds them back into the LLM, retrying until it succeeds or hits the retry limit.

---

## Mailing List Hypothesis

I’ve observed that LLMs are unusually strong at generating C compared to many other languages. My hypothesis is that this strength comes not only from C’s long history, but also from its unique documentation and development culture.

For decades, the largest C ecosystems—Linux, FreeBSD, OpenBSD, NetBSD, the GNU project, and POSIX—have relied heavily on **public mailing lists** for development. These archives contain not just code, but rationale, patch reviews, corrections, portability notes, and security discussions. Man pages, in turn, encode the interface contracts of these systems in a terse, regularized form.

Together, they form a feedback loop:

**Specification (man pages) → Discussion (mailing lists) → Implementation (patches) → Correction/Review → Updated specification**

This cycle produces training data that is:

* **Rich in context** (not just the “what” but the “why”)
* **Self-correcting** (errors are publicly fixed and explained)
* **Grounded in constraints** (performance, security, portability, backwards compatibility)
* **Iterative** (patches evolve through multiple revisions with explicit reasoning)

### Why this matters for LLMs

Unlike isolated code snippets, mailing list discussions and man pages encode *reasoning patterns*. They capture not only solutions but the thought processes behind them. That kind of explanatory signal is exactly what makes LLMs better at “thinking in C” rather than just emitting syntax.

### Supporting ecosystems

* **Linux Kernel (LKML):** Millions of messages discussing design trade-offs
* **FreeBSD:** Long-running technical lists with detailed rationale
* **OpenBSD:** Security-first reviews and audits
* **NetBSD:** Architecture and portability debates
* **GNU Project:** Coreutils, GCC, and tooling development
* **POSIX Standards:** Formal specifications with illustrative examples

**The Hypothesis:** Languages with strong text-based development cultures (especially mailing lists + man pages) provide LLMs with superior training data because they encode both code *and* intent. For C, this creates a uniquely fertile ground for models to learn not just syntax, but systems-level reasoning.

MPC leverages this: a well-written man page acts like the distilled version of those archives, giving the model exactly what it needs to map specification → correct, idiomatic C.

---

## Examples

```bash
mpc examples/hello.1      # Simple greeting program
mpc examples/count.1      # Line counter
mpc examples/calculator.1 # Add two numbers
```

See the `examples/` directory for more. Details in `man mpc`.

---

## License

BSD 3-Clause License.
