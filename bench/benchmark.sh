#!/bin/bash

# Benchmark script to compare Rust vs MPC compilation speeds
# Compares traditional rustc compilation against MPC's LLM-powered compilation

set -e

BENCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$BENCH_DIR")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🏁 MPC vs Rust Compilation Benchmark${NC}"
echo "=============================================="
echo

# Check dependencies
check_deps() {
    echo "🔍 Checking dependencies..."

    if ! command -v hyperfine &> /dev/null; then
        echo -e "${RED}❌ hyperfine not found. Install with: brew install hyperfine${NC}"
        exit 1
    fi

    if ! command -v rustc &> /dev/null; then
        echo -e "${RED}❌ rustc not found. Install with: curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh${NC}"
        exit 1
    fi

    # Check if mpc is available
    if ! command -v mpc &> /dev/null && [ ! -f "$PROJECT_ROOT/bin/mpc" ]; then
        echo -e "${YELLOW}⚠️  mpc not found. Building...${NC}"
        cd "$PROJECT_ROOT"
        make clean && make
        cd "$BENCH_DIR"
    fi

    echo -e "${GREEN}✓ All dependencies available${NC}"
    echo
}

# Setup benchmark environment
setup() {
    echo "🛠️  Setting up benchmark environment..."

    # Clean any existing binaries
    rm -f "$BENCH_DIR/calculator-rust" "$BENCH_DIR/calculator-mpc"
    rm -f "$PROJECT_ROOT/bin/calculator"

    # Ensure we have the source files
    if [ ! -f "$BENCH_DIR/calculator.rs" ]; then
        echo -e "${RED}❌ calculator.rs not found in bench directory${NC}"
        exit 1
    fi

    if [ ! -f "$PROJECT_ROOT/examples/calculator.1" ]; then
        echo -e "${RED}❌ calculator.1 not found in examples directory${NC}"
        exit 1
    fi

    echo -e "${GREEN}✓ Environment ready${NC}"
    echo
}

# Rust compilation command
rust_compile() {
    rustc -O "$BENCH_DIR/calculator.rs" -o "$BENCH_DIR/calculator-rust"
}

# MPC compilation command
mpc_compile() {
    cd "$PROJECT_ROOT"
    # Source .envrc to get PATH with ./bin
    if [ -f .envrc ]; then
        source .envrc >/dev/null 2>&1
    fi
    if command -v mpc &> /dev/null; then
        mpc -o "$BENCH_DIR/calculator-mpc" examples/calculator.1
    else
        ./bin/mpc -o "$BENCH_DIR/calculator-mpc" examples/calculator.1
    fi
    cd "$BENCH_DIR"
}

# Verify both binaries work correctly
verify_binaries() {
    echo "🧪 Verifying compiled binaries work correctly..."

    # Test Rust binary
    if [ -f "calculator-rust" ]; then
        rust_result=$(./calculator-rust 42 13)
        if [ "$rust_result" != "55" ]; then
            echo -e "${RED}❌ Rust binary test failed: expected 55, got $rust_result${NC}"
            exit 1
        fi
        echo -e "${GREEN}✓ Rust binary works correctly${NC}"
    else
        echo -e "${RED}❌ Rust binary not found${NC}"
        exit 1
    fi

    # Test MPC binary
    if [ -f "calculator-mpc" ]; then
        mpc_result=$(./calculator-mpc 42 13 2>/dev/null || echo "FAILED")
        if [ "$mpc_result" = "FAILED" ]; then
            echo -e "${YELLOW}⚠️  MPC binary failed to run (LLM generation issue)${NC}"
            return 1
        elif [ "$mpc_result" != "55" ]; then
            echo -e "${YELLOW}⚠️  MPC binary test failed: expected 55, got $mpc_result${NC}"
            return 1
        else
            echo -e "${GREEN}✓ MPC binary works correctly${NC}"
        fi
    else
        echo -e "${RED}❌ MPC binary not found${NC}"
        return 1
    fi

    return 0
}

# Run the benchmark
run_benchmark() {
    echo "🚀 Running compilation speed benchmark..."
    echo

    # Warmup runs
    echo "🔥 Warming up..."
    rust_compile > /dev/null 2>&1
    rm -f calculator-rust

    # Try MPC warmup (might fail due to LLM variability)
    if mpc_compile > /dev/null 2>&1; then
        rm -f calculator-mpc
        mpc_available=true
    else
        echo -e "${YELLOW}⚠️  MPC warmup failed - LLM might be unavailable${NC}"
        mpc_available=false
    fi

    echo

    if [ "$mpc_available" = true ]; then
        # Run hyperfine benchmark
        hyperfine \
            --warmup 1 \
            --runs 3 \
            --setup 'rm -f calculator-rust calculator-mpc' \
            --export-markdown results.md \
            --export-json results.json \
            'rustc -O calculator.rs -o calculator-rust' \
            'cd .. && source .envrc >/dev/null 2>&1 && (command -v mpc >/dev/null && mpc -o bench/calculator-mpc examples/calculator.1 || ./bin/mpc -o bench/calculator-mpc examples/calculator.1) && cd bench'
    else
        echo -e "${YELLOW}⚠️  Skipping MPC benchmark due to LLM unavailability${NC}"
        echo "Running Rust-only benchmark for reference:"
        hyperfine \
            --warmup 2 \
            --runs 5 \
            --setup 'rm -f calculator-rust' \
            'rustc -O calculator.rs -o calculator-rust'
    fi
}

# Show results summary
show_results() {
    echo
    echo "📊 Results Summary"
    echo "=================="

    if [ -f "results.md" ]; then
        echo "Detailed results saved to: $BENCH_DIR/results.md"
        echo "JSON data saved to: $BENCH_DIR/results.json"
        echo
        cat results.md
    fi

    # Show binary sizes
    echo
    echo "📏 Binary Sizes:"
    if [ -f "calculator-rust" ]; then
        rust_size=$(wc -c < calculator-rust)
        echo "  Rust binary: $rust_size bytes"
    fi
    if [ -f "calculator-mpc" ]; then
        mpc_size=$(wc -c < calculator-mpc)
        echo "  MPC binary:  $mpc_size bytes"
    fi

    # Verify functionality one more time
    echo
    if verify_binaries; then
        echo -e "${GREEN}✅ Both implementations produce correct results!${NC}"
    fi

    echo
    echo "🤔 Analysis:"
    echo "• Rust compilation is optimized, predictable, and very fast"
    echo "• MPC includes LLM generation time (network + inference)"
    echo "• MPC generates C code that's then compiled with clang/gcc"
    echo "• Trade-off: Write documentation vs write code"
}

# Cleanup
cleanup() {
    echo
    echo "🧹 Cleaning up..."
    rm -f calculator-rust calculator-mpc
}

# Main execution
main() {
    check_deps
    setup
    run_benchmark
    show_results
    cleanup

    echo
    echo -e "${BLUE}🏁 Benchmark complete!${NC}"
    echo "Run again with: ./bench/benchmark.sh"
}

# Handle Ctrl+C
trap cleanup EXIT

# Run if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
