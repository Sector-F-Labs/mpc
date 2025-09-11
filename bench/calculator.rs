use std::env;
use std::process;

const VERSION: &str = "1.0.0";

fn main() {
    let args: Vec<String> = env::args().collect();

    // Handle special flags first
    if args.len() >= 2 {
        match args[1].as_str() {
            "-h" | "--help" => {
                print_help();
                process::exit(0);
            }
            "-v" | "--version" => {
                println!("calculator {}", VERSION);
                process::exit(0);
            }
            _ => {}
        }
    }

    // Check for quiet flag
    let mut quiet = false;
    let mut numbers: Vec<String> = Vec::new();

    for arg in &args[1..] {
        match arg.as_str() {
            "-q" | "--quiet" => {
                quiet = true;
            }
            _ => {
                numbers.push(arg.clone());
            }
        }
    }

    // Validate argument count
    if numbers.len() != 2 {
        eprintln!("Error: Expected exactly 2 arguments, got {}", numbers.len());
        process::exit(2);
    }

    // Parse first number
    let num1 = match numbers[0].parse::<i64>() {
        Ok(n) => n,
        Err(_) => {
            eprintln!("Error: Invalid integer: '{}'", numbers[0]);
            process::exit(1);
        }
    };

    // Parse second number
    let num2 = match numbers[1].parse::<i64>() {
        Ok(n) => n,
        Err(_) => {
            eprintln!("Error: Invalid integer: '{}'", numbers[1]);
            process::exit(1);
        }
    };

    // Check for overflow
    let result = match num1.checked_add(num2) {
        Some(r) => r,
        None => {
            eprintln!("Error: Integer overflow");
            process::exit(1);
        }
    };

    // Output result
    println!("{}", result);

    process::exit(0);
}

fn print_help() {
    println!("Usage: calculator [OPTIONS] <number1> <number2>");
    println!("Add two integers and output the sum.");
    println!();
    println!("Options:");
    println!("  -h, --help     Display this help message");
    println!("  -v, --version  Display version information");
    println!("  -q, --quiet    Suppress verbose output");
    println!();
    println!("Examples:");
    println!("  calculator 42 13    # outputs: 55");
    println!("  calculator -5 10    # outputs: 5");
}
