import subprocess
import os
import sys
import random

def compile_if_needed(src_file, exe_file, compiler, flags):
    need_compile = False
    if not os.path.exists(exe_file):
        need_compile = True
    else:
        # Check modification times
        if os.path.getmtime(src_file) > os.path.getmtime(exe_file):
            need_compile = True
    if need_compile:
        print(f"Compiling {src_file}...")
        # Split the flags into a list
        cmd = [compiler, src_file, "-o", exe_file] + flags.split()
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"Compilation of {src_file} successful.")
        else:
            print(f"Error compiling {src_file}. Exiting.")
            print(result.stderr)
            sys.exit(1)
    else:
        print(f"{exe_file} is up to date.")

def random_float(min_val, max_val):
    return random.uniform(min_val, max_val)

def main():
    # --- Configuration ---
    MPI_SRC = "2D_HeatEquation_MPI.cpp"
    MPI_EXE = "./heat_equation_2d_mpi.exe"
    CXXFLAGS_COMMON = "-O3 -Wall -std=c++17"

    print("--- 2D Heat Equation Simulation Runner (Python with Random Boundaries) ---")
    
    # Get parameters from user
    try:
        n_inner_input = input("Enter N_INNER (e.g., 100, default: 100): ")
    except EOFError:
        n_inner_input = ""
    N_INNER = int(n_inner_input) if n_inner_input.strip() != "" else 100

    max_iter_input = input("Enter MAX_ITERATIONS (e.g., 1000, default: 1000): ")
    MAX_ITERATIONS = int(max_iter_input) if max_iter_input.strip() != "" else 1000

    num_procs_input = input("Enter number of MPI processes (e.g., 4, default: 4): ")
    NUM_PROCS = int(num_procs_input) if num_procs_input.strip() != "" else 4

    runs_input = input("Enter number of simulation runs (default: 1): ")
    NUM_RUNS = int(runs_input) if runs_input.strip() != "" else 1

    # Generate random boundary values (0.0 to 100.0)
    BOUNDARY_TOP = random_float(0.0, 100.0)
    BOUNDARY_BOTTOM = random_float(0.0, 100.0)
    BOUNDARY_LEFT = random_float(0.0, 100.0)
    BOUNDARY_RIGHT = random_float(0.0, 100.0)

    print("Using Random Boundary Conditions:")
    print(f"  Top:    {BOUNDARY_TOP}")
    print(f"  Bottom: {BOUNDARY_BOTTOM}")
    print(f"  Left:   {BOUNDARY_LEFT}")
    print(f"  Right:  {BOUNDARY_RIGHT}")

    print("\n--- Compilation Check ---")
    compile_if_needed(MPI_SRC, MPI_EXE, "mpic++", CXXFLAGS_COMMON)

    print("\n--- Running Simulations & Recording Program-Reported Times ---")

    # Run MPI Program NUM_RUNS times and average its reported time
    MPI_TOTAL = 0.0
    cmd = [
        "mpiexec",
        "-n", str(NUM_PROCS),
        MPI_EXE,
        str(N_INNER),
        str(MAX_ITERATIONS),
        str(BOUNDARY_TOP),
        str(BOUNDARY_BOTTOM),
        str(BOUNDARY_LEFT),
        str(BOUNDARY_RIGHT)
    ]
    print(f"\nRunning MPI Program ({' '.join(cmd)}) for {NUM_RUNS} runs...")
    for i in range(1, NUM_RUNS + 1):
        result = subprocess.run(cmd, capture_output=True, text=True)
        mpi_out = result.stdout.strip()
        # Clean output: take the first non-empty line (assuming rank 0 prints the time)
        cleaned_out = ""
        for line in mpi_out.splitlines():
            if line.strip():
                cleaned_out = line.strip().split()[0]
                break
        print(f"  Run {i}: {cleaned_out}")
        try:
            MPI_TOTAL += float(cleaned_out)
        except ValueError:
            print("Error: Could not convert simulation output to float.")
            sys.exit(1)
    MPI_AVG = MPI_TOTAL / NUM_RUNS
    print(f"MPI Program Average Execution Time: {MPI_AVG:.6f}")

    print("\n--- Script Finished ---")

if __name__ == "__main__":
    main()
