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



## Example Programs

The `examples/` directory contains several working examples:

### Simple Greeting (`hello.1`)
```bash
mpc examples/hello.1
./hello                    # "Hello, World!"
./hello Alice              # "Hello, Alice!"
./hello -h                 # Show help
```

### File Counter (`count.1`)
```bash
mpc examples/count.1
echo "test" | ./count      # Count lines from stdin
./count file.txt           # Count lines in file
```

### Calculator (`calculator.1`)
```bash
mpc examples/calculator.1
./calculator 42 13         # Outputs: 55
./calculator --help        # Show usage
```

## LLM Backends

### Ollama (Local)
**Pros:**
- **Complete privacy (runs locally)
- No API costs
- Works offline
- Fast response times
- **Excellent on macOS** with Apple Silicon

**Setup:**
```bash
# Install Ollama (macOS - easiest)
brew install ollama

# Or use official installer
curl -fsSL https://ollama.ai/install.sh | sh

# Pull a code model optimized for C generation
ollama pull codellama:7b      # 4GB download - excellent for C
ollama pull deepseek-coder    # Alternative - also strong at C

# Start server
ollama serve
```

**Recommended models:**
- `codellama:7b` - Good balance of speed/quality
- `deepseek-coder:6.7b` - Excellent for code generation
- `codellama:13b` - Better quality, slower

### Gemini API (Remote)
**Pros:**
- No local setup required
- Latest Google AI models
- Potentially higher quality

**Setup:**
```bash
# Get API key from Google AI Studio
export GEMINI_API_KEY=your_key_here

# Use with --remote flag
mpc --remote examples/simple.1
```

**Available models:**
- `gemini-1.5-flash` - Fast, good quality (default)
- `gemini-1.5-pro` - Higher quality, slower, excellent C code

## Implementation Details

MPC is implemented in **clean, portable C** (~500 lines):

- **Optimized for macOS** - Uses Clang by default, Homebrew packages
- **No complex dependencies** - Just libcurl and jansson
- **Fast compilation** - Builds in seconds on Apple Silicon
- **Easy to understand** - Straightforward C anyone can read
- **Cross-platform** - Works on macOS, Linux, Windows (WSL)
- **Self-contained** - No runtime dependencies



## Testing

**Run built-in tests:**
```bash
make test
```

**Compile all examples:**
```bash
make examples               # Generate all example binaries to ./bin/
make clean-examples         # Remove example binaries (keep mpc)
```

**Test specific example:**
```bash
mpc -v examples/simple.1
./simple --help
```

**Debug a generation:**
```bash
mpc -d -v examples/calculator.1
cat build/calculator.c    # See generated code
```

## Advanced Features

### Debug Mode
See exactly what's happening:
```bash
mpc -d examples/simple.1
```
- Shows generated specification
- Displays raw LLM response
- Saves generated source code
- Shows compilation commands

### Custom Models
```bash
# Local Ollama - try different C-trained models
mpc -m deepseek-coder examples/simple.1    # Excellent for C
mpc -m codellama:13b examples/simple.1     # Larger model, better quality

# Remote Gemini - Google's latest models
mpc --remote -m gemini-1.5-pro examples/simple.1
```

## Troubleshooting

Having issues? See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for solutions to common problems including:
- Installation and dependency issues
- Ollama connection problems
- Code generation failures
- Debug mode usage

## Development

### Building
```bash
make clean
make
make debug      # Debug build with symbols
make examples   # Compile all example man pages to ./bin/
```

### Contributing
1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Submit a pull request

## Limitations

### Current Limitations
- **LLM Quality**: Output depends on model capabilities (but C generation is excellent!)
- **Complexity**: Works best for standard CLI patterns and POSIX tools
- **Review Required**: Generated code should be tested (though usually works)
- **Platform**: Primarily tested on macOS and Linux













## License

BSD 3-Clause License.











---

**MPC: Documentation is the source; code is the build artifact.**

*Write the man page. Run the result.*
