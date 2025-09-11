# Troubleshooting

## Installation Issues

**Dependencies missing:**
```bash
# Run configure script to auto-install
./configure

# Or install manually:
# macOS: brew install curl jansson ollama
# Ubuntu: sudo apt-get install libcurl4-openssl-dev libjansson-dev
```

**Build fails:**
```bash
make clean && make
```

## Runtime Issues

**Ollama connection fails:**
```bash
ollama serve                    # Start server
ollama pull codellama:7b       # Download model
```

**Use remote API instead:**
```bash
export GEMINI_API_KEY=your_key
mpc --remote examples/simple.1
```

## Code Generation Issues

**Generated code won't compile:**
```bash
mpc -r 5 -v examples/simple.1  # Retry with error feedback
mpc -m deepseek-coder examples/simple.1  # Try different model
mpc --remote examples/simple.1  # Use remote API
```

**Generated program doesn't work:**
- Add more detailed examples to your man page
- Include expected output in EXAMPLES section
- Specify argument types clearly in SYNOPSIS
- Follow standard man page format

## Debug Mode

```bash
mpc -d -v examples/simple.1    # Enable debug output
cat build/simple.c             # Inspect generated code
```

## Getting Help

Include this info when reporting issues:
- Command used: `mpc -v examples/simple.1`
- System: `uname -a`
- Contents of `build/` directory (if using `-d`)