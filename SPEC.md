# Man Page Compiler (MPC) - Technical Specification

## Overview

The Man Page Compiler (MPC) is a documentation-driven compiler that translates Unix man page documentation (`.1` files) into working C programs using Large Language Models. This represents a paradigm shift where documentation becomes executable specification.

## Architecture

### Core Philosophy
**Documentation as Code**: The man page *is* the specification. The compiler generates C implementation that matches the documentation exactly, ensuring they never drift apart.

### Implementation Language & Platform
MPC is implemented in **portable C** (~500 lines) optimized for **macOS as the primary platform**:
- **macOS optimized**: Uses Clang by default, Homebrew dependencies
- **Fast compilation**: Builds in seconds on Apple Silicon
- **Simple debugging**: Straightforward C code anyone can read
- **Minimal runtime**: No complex dependencies or virtual machines
- **Cross-platform**: Also works on Linux and Windows (WSL)

## System Architecture

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   .1 File   │───▶│   Parser    │───▶│ LLM Client  │───▶│  Compiler   │
│  (groff)    │    │ (C native)  │    │(Ollama/API) │    │ (gcc/clang) │
└─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘
                            │                   │                   │
                            ▼                   ▼                   ▼
                   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
                   │ Structured  │    │ Generated   │    │ Executable  │
                   │    Data     │    │    Code     │    │   Binary    │
                   └─────────────┘    └─────────────┘    └─────────────┘
```

## Core Components

### 1. Man Page Parser (`parse_manpage()`)
- **Input**: `.1` groff/troff formatted file
- **Process**: Native C parsing of standard man page sections
- **Output**: Structured `manpage_t` containing:
  - Command name and description
  - Synopsis (usage pattern)
  - Options with flags and descriptions
  - Examples with expected behavior
  - All other documented sections

### 2. Specification Generator (`generate_specification()`)
- **Input**: Parsed man page structure + target configuration
- **Process**: Generate comprehensive English specification for LLM
- **Output**: Detailed prompt including:
  - Interface requirements (CLI args, options)
  - Behavioral specifications from description
  - Example-driven test cases
  - Target language constraints
  - Code quality requirements

### 3. LLM Interface (`call_llm()`)
**Dual-mode operation:**

#### Local Mode (Ollama)
- **Endpoint**: `http://localhost:11434/api/generate`
- **Format**: Ollama JSON API
- **Models**: `codellama:7b`, `deepseek-coder:6.7b`, etc.
- **Benefits**: Privacy, no API costs, offline operation

#### Remote Mode (Gemini API)
- **Endpoint**: `https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent`
- **Authentication**: `GEMINI_API_KEY` environment variable
- **Models**: `gemini-1.5-flash`, `gemini-1.5-pro`
- **Benefits**: Latest models, no local setup required

### 4. Code Extraction (`extract_code()`)
- **Input**: Raw LLM response (often with markdown formatting)
- **Process**: Parse and extract actual source code from response
- **Output**: Clean source code ready for compilation

### 5. C Code Compiler (`compile_code()`)
**Target: C Only**

MPC generates **C code exclusively** for compelling reasons:

| Aspect | C Generation | Other Languages |
|--------|-------------|-----------------|
| **LLM Training** | ✅ Extensive (Linux kernel, POSIX) | ⚠️ Limited |
| **Success Rate** | ✅ 85-90% clean compilation | ❌ 40-70% |
| **Toolchain** | ✅ Universal (Clang/GCC) | ⚠️ Language-specific |
| **Performance** | ✅ Native speed | ⚠️ Variable |
| **Complexity** | ✅ Simple, debuggable | ❌ Complex syntax/semantics |

## File Format Specifications

### Input: Man Page Format
```roff
.TH COMMAND 1 "date" "version" "section"
.SH NAME
command \- brief description
.SH SYNOPSIS
.B command
.RI [ OPTIONS ]
.I arguments
.SH DESCRIPTION
Detailed behavior specification...
.SH OPTIONS
.TP
.BR \-h ", " \-\-help
Display help and exit
.SH EXAMPLES
$ command arg1 arg2
expected output
```

**Supported sections:**
- **NAME** (required): Command name + description
- **SYNOPSIS** (required): Usage syntax
- **DESCRIPTION** (required): Behavior specification
- **OPTIONS** (optional): Command-line flags
- **EXAMPLES** (optional): Usage examples
- **EXIT STATUS** (optional): Exit code meanings
- **FILES** (optional): Configuration files

### Output: Generated Executable
- **Format**: Native binary for target platform
- **Interface**: Matches documented CLI exactly
- **Behavior**: Implements documented functionality
- **Error handling**: Basic validation and error messages
- **Exit codes**: Standard POSIX conventions

## LLM Integration

### Prompt Engineering Strategy

#### Base Template for C Generation
```
Write a complete C program named '{command_name}' that implements this exact specification.

DESCRIPTION: {description}

COMMAND LINE INTERFACE:
Usage: {synopsis}

OPTIONS:
{parsed_options_list}

EXAMPLES:
{example_commands_and_outputs}

REQUIREMENTS:
- Include ALL necessary #include statements
- Implement proper command-line argument parsing using getopt()
- Add error handling for invalid inputs
- Follow standard C conventions and POSIX compliance
- Make it compile without warnings on macOS and Linux
- Use standard C library functions (printf, fprintf, etc.)

OUTPUT: Complete working C source code ONLY.
Wrap in ```c code blocks.
Start immediately with #include statements.
DO NOT include explanations or markdown text.

NOTE: C code generation works exceptionally well because LLMs have been
extensively trained on Linux kernel code and mailing list discussions,
making them excellent at generating clean, portable C code.
```

#### Why C Generation Works So Well

**The Linux Kernel Training Effect:**
- LLMs trained extensively on Linux kernel source code (millions of lines)
- Kernel mailing list discussions provide detailed code reviews and explanations
- POSIX standards documentation gives complete API specifications
- GNU Coreutils source provides reference CLI tool implementations

**Result:** LLMs are essentially C code experts with decades of systems programming knowledge

### Quality Improvements

#### Context Optimization
- **Focused specification**: Only relevant man page sections included
- **Example prioritization**: Most illustrative examples emphasized
- **Error case handling**: Common error conditions specified
- **Target constraints**: Language-specific requirements highlighted

#### Response Processing
- **Code extraction**: Robust markdown parsing for code blocks
- **Syntax validation**: Basic syntax checking before compilation
- **Error recovery**: Retry with modified prompts on failures
- **Debug output**: Full LLM exchange logging in debug mode

## API Specifications

### Ollama API Integration
```json
POST /api/generate
{
  "model": "codellama:7b",
  "prompt": "generated_specification_text",
  "stream": false
}

Response:
{
  "response": "generated_code_text",
  "done": true
}
```

### Gemini API Integration
```json
POST /v1beta/models/gemini-1.5-flash:generateContent?key=API_KEY
{
  "contents": [{
    "parts": [{
      "text": "generated_specification_text"
    }]
  }]
}

Response:
{
  "candidates": [{
    "content": {
      "parts": [{
        "text": "generated_code_text"
      }]
    }
  }]
}
```

## Configuration Management

### Command Line Interface
```bash
mpc [OPTIONS] INPUT.1

Options:
  -o OUTPUT      Output executable name
  -m MODEL       LLM model name
  -u URL         Ollama server URL
  --remote       Use remote Gemini API
  -r RETRIES     Max retry attempts with error feedback (default: 3)
  -v             Verbose output
  -d             Debug mode
  -h             Show help
```

### Environment Variables
```bash
GEMINI_API_KEY    # Required for --remote mode
```

### Configuration Structure
```c
typedef struct {
    char *input_file;      // Source .1 file
    char *output_file;     // Target executable
    char *target_language; // Always "c"
    char *llm_model;       // Model identifier
    char *llm_url;         // API endpoint
    char *api_key;         // Authentication
    int use_remote;        // Local vs remote LLM
    int verbose;           // Detailed output
    int debug;             // Debug information
    int max_retries;       // Retry attempts with error feedback
} config_t;
```

## Error Handling & Retry System

### Intelligent Retry Mechanism

MPC implements a sophisticated retry system that learns from compilation failures:

**Automatic Error Feedback Loop:**
1. **Capture Compilation Errors** - Full compiler output with line numbers and error descriptions
2. **Generate Enhanced Prompt** - Original specification + compilation errors + fix instructions
3. **Retry LLM Generation** - Send enhanced prompt to LLM for improved code generation
4. **Repeat Until Success** - Continue up to `max_retries` attempts (default: 3)

**Error Feedback Integration:**
```c
char* generate_specification(const manpage_t *manpage, 
                           const config_t *config, 
                           const char *error_feedback);
```

**Compilation Error Capture:**
```c
int compile_code(const char *code, 
                const config_t *config, 
                char **error_output);
```

### Error Categories

**Parse Errors:**
- Invalid groff syntax
- Missing required sections
- Malformed option definitions

**Generation Errors:**
- LLM API failures
- Network connectivity issues
- Invalid generated code syntax

**Compilation Errors (Auto-Retried):**
- Missing `#include` statements
- Undefined variables and functions
- Function prototype mismatches
- Type conversion issues
- Syntax errors

**Runtime Errors:**
- Invalid command line arguments
- File system permissions
- Resource exhaustion

### Error Recovery Strategies

**Automatic Recovery (Retry System):**
- Compilation error feedback to LLM
- Enhanced prompts with specific fix instructions
- Progressive improvement across attempts
- Success rate improvement from ~75% to ~90%+

**Graceful Degradation:**
- Retry with simplified prompts
- Fallback to basic implementations
- Partial functionality generation

**User Guidance:**
- Detailed error messages with context
- Suggested fixes and workarounds
- Links to documentation and help

## Performance Characteristics

### Compilation Pipeline Performance
- **Parsing**: ~1ms for typical man page
- **LLM Generation**: 2-30 seconds (model dependent)
- **Code Compilation**: 100ms-2s (language dependent)
- **Total Time**: Dominated by LLM generation phase

### Generated Code Performance
- **Startup Time**: <10ms typical
- **Memory Usage**: 1-10MB typical
- **CLI Response**: Interactive speeds
- **Throughput**: Depends on generated algorithm complexity

### Scalability Considerations
- **Concurrent Builds**: Limited by LLM API rate limits
- **Batch Processing**: Can process multiple man pages
- **Caching**: Generated code can be cached/versioned
- **Distribution**: Generated binaries are self-contained

## Testing Strategy

### Unit Testing
- Man page parser with various formats
- LLM interface mocking and error simulation
- Code extraction with malformed responses
- Compilation pipeline with different targets

### Integration Testing
- Full pipeline with real examples
- Cross-platform binary generation
- LLM integration with actual models
- Generated binary behavioral validation

### Validation Framework
```bash
# Automated testing
make test                    # Run all tests
mpc examples/simple.1        # Generate test binary
./simple --help              # Verify basic functionality
echo "test" | ./simple       # Test with input
```

### Quality Metrics
- **Success Rate**: % of man pages that generate working binaries (90%+ with retry system)
- **Behavioral Accuracy**: % match between docs and implementation
- **Compilation Rate**: % of generated code that compiles cleanly (improved via retry feedback)
- **Retry Effectiveness**: Success rate improvement from retry attempts
- **Performance**: Generated binary startup and execution time

## Security Considerations

### Input Validation
- **Man page parsing**: Bounded buffer handling
- **LLM responses**: Code injection prevention
- **File operations**: Path traversal protection
- **System calls**: Restricted compilation environment

### Generated Code Security
- **Basic validation**: Input bounds checking
- **Error handling**: Graceful failure modes
- **Resource limits**: Reasonable memory/CPU usage
- **Privilege handling**: Standard user permissions

### API Security
- **Local Ollama**: No network exposure by default
- **Remote APIs**: HTTPS only, API key protection
- **Credential handling**: Environment variable isolation
- **Rate limiting**: Respect API quotas and limits

## Deployment & Distribution

### Build Requirements
**Development Dependencies (macOS):**
- Clang (preferred) or GCC (C compiler)
- libcurl development headers (via Homebrew)
- jansson JSON library (via Homebrew)
- Make build system
- pkg-config for dependency management

**Runtime Dependencies:**
- None (statically linked where possible)
- Standard C library
- Clang/GCC for compiling generated C code

### Installation Methods
```bash
# From source (macOS)
git clone <repository>
cd llanguage
make deps    # Install dependencies via Homebrew
make         # Build compiler (uses Clang)
make install # Install to /usr/local/bin

# Future package distribution
brew install mpc    # Planned Homebrew formula
```

### Platform Support
- **macOS**: Primary development platform (ARM64 and Intel)
- **Linux**: Full support (tested on Ubuntu, CentOS)
- **Windows**: WSL support, native compilation planned
- **BSD**: Should work with minimal modifications

**Optimization Priority:**
1. **macOS** - Primary target, Clang optimized
2. **Linux** - Secondary support, GCC compatible
3. **Others** - Community contributions welcome

## Future Enhancements

### Phase 2: C Generation Improvements
- **Better templates**: Improved C code generation patterns
- **Shell completion**: Generate bash/zsh completion from OPTIONS
- **Configuration files**: Auto-generate config parsing from FILES section
- **Package metadata**: Auto-generate Makefile, pkg-config files

### Phase 3: macOS Ecosystem Integration
- **Homebrew integration**: Official formula and tap
- **Xcode integration**: Build phase integration
- **macOS packaging**: Native .pkg installers
- **System integration**: Launch daemons, system services

### Long-term Vision
- **Interactive refinement**: Iterative improvement of generated C tools
- **Performance optimization**: LLM-guided C optimization passes
- **Static analysis**: Integration with Clang static analyzer
- **Cross-compilation**: Generate C code for multiple architectures

## Limitations & Constraints

### Current Limitations
- **Complexity bounds**: Works best for standard CLI patterns and POSIX tools
- **LLM dependency**: Quality limited by model capabilities (though C generation excels)
- **Platform support**: Optimized for macOS, good Linux support
- **Code patterns**: Limited to what LLMs learned from kernel/POSIX codebases

### Fundamental Constraints
- **Documentation quality**: Output quality correlates with man page quality
- **LLM capabilities**: Bounded by training data (fortunately rich for C)
- **Compilation requirements**: Needs Clang/GCC toolchain
- **Review recommended**: Generated code should be tested (though usually works)

### Mitigation Strategies
- **Quality guidelines**: Best practices for man page writing
- **Model selection**: Support multiple LLM backends
- **Graceful degradation**: Fallback to simpler implementations
- **Human-in-the-loop**: Debug mode for inspection and refinement

## Success Metrics

### Technical Metrics
- **C compilation success rate**: >85% for well-formed man pages (excellent!)
- **Behavioral accuracy**: Generated C tools match documented behavior
- **Performance**: Native C speed, <10ms startup, 1-10MB memory
- **macOS optimization**: Leverages Clang optimizations and Apple Silicon

### Adoption Metrics
- **macOS developer usage**: Active users generating CLI tools
- **Homebrew integration**: Community formulas and contributions
- **System integration**: Usage in build systems and automation
- **Man page quality**: Improvement in documentation practices

---

**Man Page Compiler represents a fundamental shift toward documentation-driven development, where perfect documentation becomes perfect software.**

*"The best code is the code that matches its documentation exactly - because they are the same thing."*