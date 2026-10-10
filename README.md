# Man Page Compiler (MPC)

Turn a Unix man page (`.1`) into a real, working CLI program. You write the documentation first; MPC generates the implementation to match it.

## What is this?

MPC is a **documentation-as-source compiler**: the man page is treated as the canonical specification, and the generated code is built to conform to it.

## Philosophy: Inverting the Usual Direction

Traditional workflow: write code → (maybe) write docs → docs drift out of sync.

MPC workflow: write precise docs → generate matching code → docs remain the source of truth.

Man pages already define interface, behavior, options, examples, and exit codes. MPC treats this as an *executable specification* and produces C programs that implement it.

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

## Quick Start

```bash
./configure                    # Auto-setup everything
./bin/mpc examples/simple.1    # Generate program
./simple                       # Run it
```

The `./configure` script installs dependencies, sets up Ollama, builds MPC, and runs a quick test.

## Setup

```bash
./configure
```

That single command:

* Installs prerequisites
* Builds MPC
* Fetches a default local model (`gemma4` via Ollama)
* Runs a sanity check

Ollama on another machine?

```bash
mpc -u http://gpu-box:11434 examples/simple.1
```

See `SETUP.md` for optional platform-specific notes.

## Usage

```bash
mpc examples/simple.1    # Generate from man page
./simple --help          # Run the result
```

Run `mpc --help` for available options.

## How It Works

```
Man Page → Parse → LLM → Generated Code → Executable
```

When compilation fails, MPC captures compiler errors and feeds them back into the LLM, retrying until it succeeds or hits the retry limit.

## Mailing List Hypothesis

I’ve observed that LLMs are unusually strong at generating C compared to many other languages. Often, single-shot generation of small C programs is successful, whereas languages like Rust usually require several iterations. My hypothesis is that this strength comes not only from the abundance of C in training data, but also from its development and documentation culture.

Large C projects—Linux, FreeBSD, OpenBSD, NetBSD, GNU, POSIX—are rooted in **public mailing lists** and **man pages**. Mailing lists preserve structured technical discussion: patches, rationale, corrections, portability notes, and security reviews. Man pages encode the public interface in a terse, regular format, often accompanied by examples.

Together, these sources create a closed loop of specification → discussion → implementation → correction → updated specification. This produces training material that includes both the code and the reasoning behind it. That conversational, explanatory signal mirrors the fine-tuning methods used in LLM training, making models better at “thinking in C.”

## Narrow Scope Advantage

MPC also benefits from the intentionally limited problem space: generating small, POSIX-style command-line utilities in C. The scope is narrow, the interfaces are simple, and the expected patterns are well established. This makes the task tractable for LLMs, which otherwise struggle with more complex or open-ended programming challenges.

One interesting observation is that man page examples are critical to this process. Examples provide the model with concrete input/output demonstrations, anchoring the generated implementation. Unfortunately, many Linux man pages have sparse or outdated examples, while BSD man pages tend to maintain better ones. Improving man page examples could significantly enhance the utility of MPC and similar tools.

## Examples

```bash
mpc examples/hello.1      # Simple greeting program
mpc examples/count.1      # Line counter
mpc examples/calculator.1 # Add two numbers
```

See the `examples/` directory for more. Details in `man mpc`.

## License

BSD 3-Clause License.
