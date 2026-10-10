# Makefile for Man Page Compiler (macOS/C Implementation)

CC = cc
CFLAGS = -Wall -Wextra -std=c99 -O2 -D_POSIX_C_SOURCE=200809L `pkg-config --cflags libcurl jansson`
LIBS = `pkg-config --libs libcurl jansson`

# Target executable
TARGET = bin/mpc

# Source and object files
OBJDIR = build/obj
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:src/%.c=$(OBJDIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	@mkdir -p bin
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

# Compile source files
$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

# Install system dependencies
deps:
	@echo "Installing dependencies on macOS..."
	@if command -v brew >/dev/null 2>&1; then \
		brew install curl jansson; \
	else \
		echo "Homebrew not found. Please install Homebrew first:"; \
		echo "  /bin/bash -c \"\$$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""; \
		echo "Then run: brew install curl jansson"; \
		exit 1; \
	fi

# Check if dependencies are available
check-deps:
	@echo "Checking dependencies..."
	@pkg-config --exists libcurl || (echo "Error: libcurl not found" && exit 1)
	@pkg-config --exists jansson || (echo "Error: jansson not found" && exit 1)
	@echo "✓ All dependencies found"

# Test with example
test: $(TARGET)
	@echo "Testing with simple.1 example..."
	@mkdir -p build
	@if [ -f examples/simple.1 ]; then \
		./$(TARGET) -v examples/simple.1; \
	else \
		echo "Error: examples/simple.1 not found"; \
	fi

# Compile all examples to ./bin
examples: $(TARGET)
	@echo "Compiling all examples to ./bin..."
	@mkdir -p bin
	@for example in examples/*.1; do \
		if [ -f "$$example" ]; then \
			base=$$(basename "$$example" .1); \
			echo "Compiling $$base..."; \
			./$(TARGET) -o bin/$$base "$$example" || echo "Failed to compile $$base"; \
		fi; \
	done
	@echo "✓ Examples compiled to ./bin/"
	@ls -la bin/

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: clean $(TARGET)
	@mkdir -p build

# Clean build artifacts
clean: clean-examples
	rm -f $(OBJECTS) $(DEPS) $(TARGET)
	rm -rf build/ src/build/

# Clean example binaries (but keep mpc)
clean-examples:
	@if [ -d bin ]; then \
		for example in examples/*.1; do \
			if [ -f "$$example" ]; then \
				base=$$(basename "$$example" .1); \
				rm -f bin/$$base; \
			fi; \
		done; \
		echo "✓ Cleaned example binaries from ./bin/"; \
	fi

# Install to system
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/mpc
	sudo chmod 755 /usr/local/bin/mpc
	@echo "Installed mpc to /usr/local/bin/"

# Uninstall from system
uninstall:
	sudo rm -f /usr/local/bin/mpc
	@echo "Uninstalled mpc"

# Show help
help:
	@echo "Available targets:"
	@echo "  all          - Build the compiler (default)"
	@echo "  deps         - Install system dependencies"
	@echo "  check-deps   - Check if dependencies are available"
	@echo "  test         - Build and test with example"
	@echo "  examples     - Compile all examples to ./bin"
	@echo "  debug        - Build with debug symbols"
	@echo "  clean        - Remove build artifacts"
	@echo "  clean-examples - Remove example binaries (keep mpc)"
	@echo "  install      - Install to /usr/local/bin"
	@echo "  uninstall    - Remove from /usr/local/bin"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Usage examples (macOS optimized):"
	@echo "  make deps           # Install dependencies via Homebrew"
	@echo "  make                # Build compiler (uses Clang)"
	@echo "  make test           # Test with example"
	@echo "  make examples       # Compile all examples to ./bin"
	@echo "  mpc examples/simple.1            # Generate C code and compile"
	@echo "  mpc -u http://gpu-box:11434 examples/simple.1  # Use a remote Ollama host"
	@echo ""
	@echo "LLM backend:"
	@echo "  brew install ollama && ollama pull gemma4"

.PHONY: all deps check-deps test examples debug clean clean-examples install uninstall help
