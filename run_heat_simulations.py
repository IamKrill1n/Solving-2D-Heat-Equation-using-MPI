import os
import subprocess
import time

# --- Configuration ---
SERIAL_SRC = "2d_heat_eq.cpp"
SERIAL_EXE = "2d_heat_eq.exe"
MPI_SRC = "2D_HeatEquation_MPI.cpp"
MPI_EXE = "heat_equation_2d_mpi"  # Assuming this is the typical output name for mpicxx
COMPARATOR_SRC = "compare_outputs.cpp"
COMPARATOR_EXE = "compare_outputs.exe"

OUTPUT_SERIAL_FILE = "output_serial.txt"
OUTPUT_MPI_FILE = "output_mpi.txt"

# Get project directory (assuming the script is in the project directory)
PROJECT_DIR = os.path.dirname(os.path.abspath(__file__))

SERIAL_SRC_PATH = os.path.join(PROJECT_DIR, SERIAL_SRC)
SERIAL_EXE_PATH = os.path.join(PROJECT_DIR, SERIAL_EXE)
MPI_SRC_PATH = os.path.join(PROJECT_DIR, MPI_SRC)
MPI_EXE_PATH = os.path.join(PROJECT_DIR, MPI_EXE)
COMPARATOR_SRC_PATH = os.path.join(PROJECT_DIR, COMPARATOR_SRC)
COMPARATOR_EXE_PATH = os.path.join(PROJECT_DIR, COMPARATOR_EXE)

# --- Helper Functions ---
def compile_if_needed(src_path, exe_path, compiler_cmd):
    """Compiles the source file if the executable doesn't exist or is older than the source."""
    if not os.path.exists(exe_path) or os.path.getmtime(src_path) > os.path.getmtime(exe_path):
        print(f"Compiling {os.path.basename(src_path)}...")
        try:
            compile_process = subprocess.run(compiler_cmd, check=True, capture_output=True, text=True, cwd=PROJECT_DIR)
            print(f"Compilation successful for {os.path.basename(src_path)}.")
            if compile_process.stdout:
                print("Compiler output:")
                print(compile_process.stdout)
        except subprocess.CalledProcessError as e:
            print(f"Error compiling {os.path.basename(src_path)}:")
            print(e.stderr)
            return False
    else:
        print(f"{os.path.basename(exe_path)} is up to date.")
    return True

def run_program(cmd, program_name):
    """Runs a program, measures execution time, and returns output and time."""
    print(f"\nRunning {program_name}...")
    start_time = time.time()
    try:
        process = subprocess.run(cmd, check=True, capture_output=True, text=True, cwd=PROJECT_DIR)
        end_time = time.time()
        execution_time = end_time - start_time
        print(f"{program_name} finished in {execution_time:.4f} seconds.")
        # Try to extract execution time from program's own output if available
        # This part needs to be adapted if your C++ programs print time differently
        output_lines = process.stdout.splitlines()
        for line in output_lines:
            if "execution time:" in line.lower(): # Case-insensitive search
                try:
                    # Example: "Execution time: 0.1234 s"
                    prog_reported_time_str = line.split(":")[1].strip().split()[0]
                    prog_reported_time = float(prog_reported_time_str)
                    print(f"{program_name} reported execution time: {prog_reported_time:.4f} s")
                    # If program reports time, we can use that. Otherwise, stick to script's measurement.
                    # execution_time = prog_reported_time # Uncomment to use program's time
                except (IndexError, ValueError) as e:
                    print(f"Could not parse execution time from {program_name} output: '{line}'. Error: {e}")
        return process.stdout, execution_time
    except subprocess.CalledProcessError as e:
        print(f"Error running {program_name}:")
        print(e.stderr)
        return None, 0
    except FileNotFoundError:
        print(f"Error: {program_name} executable not found at {cmd[0]}. Please compile first.")
        return None, 0

# --- Main Script ---
if __name__ == "__main__":
    print("--- 2D Heat Equation Simulation Runner (Python) ---")

    # Get parameters from user
    while True:
        try:
            n_inner_str = input(f"Enter N_INNER (e.g., 100, default: 100): ") or "100"
            n_inner = int(n_inner_str)
            if n_inner <= 0:
                raise ValueError("N_INNER must be positive.")
            break
        except ValueError as e:
            print(f"Invalid input: {e}. Please enter a positive integer.")

    while True:
        try:
            max_iter_str = input(f"Enter MAX_ITERATIONS (e.g., 1000, default: 1000): ") or "1000"
            max_iter = int(max_iter_str)
            if max_iter <= 0:
                raise ValueError("MAX_ITERATIONS must be positive.")
            break
        except ValueError as e:
            print(f"Invalid input: {e}. Please enter a positive integer.")

    while True:
        try:
            num_procs_str = input(f"Enter number of MPI processes (e.g., 4, default: 4): ") or "4"
            num_procs = int(num_procs_str)
            if num_procs <= 0:
                raise ValueError("Number of processes must be positive.")
            break
        except ValueError as e:
            print(f"Invalid input: {e}. Please enter a positive integer.")

    print("\n--- Compilation Check ---")
    # Compile serial version
    serial_compiler_cmd = ["g++", SERIAL_SRC_PATH, "-o", SERIAL_EXE_PATH, "-O3"]
    if not compile_if_needed(SERIAL_SRC_PATH, SERIAL_EXE_PATH, serial_compiler_cmd):
        print("Exiting due to serial compilation failure.")
        exit(1)

    # Compile MPI version
    # Note: mpicxx might be msmpi on Windows with MS-MPI, or require full path
    # Adjust 'mpicxx' if needed for your MPI implementation
    mpi_compiler_cmd = ["mpic++", MPI_SRC_PATH, "-o", MPI_EXE_PATH, "-O3"]
    if not compile_if_needed(MPI_SRC_PATH, MPI_EXE_PATH, mpi_compiler_cmd):
        print("Exiting due to MPI compilation failure.")
        exit(1)

    # Compile comparator (optional)
    if os.path.exists(COMPARATOR_SRC_PATH):
        comparator_compiler_cmd = ["g++", COMPARATOR_SRC_PATH, "-o", COMPARATOR_EXE_PATH, "-O3"]
        compile_if_needed(COMPARATOR_SRC_PATH, COMPARATOR_EXE_PATH, comparator_compiler_cmd)

    print("\n--- Running Simulations ---")
    # Run Serial Program
    serial_cmd = [SERIAL_EXE_PATH, str(n_inner), str(max_iter)]
    serial_output, serial_time = run_program(serial_cmd, "Serial Program")

    # Run MPI Program
    # Note: mpiexec might be msmpi on Windows with MS-MPI, or require full path
    # Adjust 'mpiexec' if needed for your MPI implementation
    mpi_cmd = ["mpiexec", "-n", str(num_procs), MPI_EXE_PATH, str(n_inner), str(max_iter)]
    mpi_output, mpi_time = run_program(mpi_cmd, "MPI Program")

    print("\n--- Execution Summary ---")
    if serial_output is not None:
        print(f"Serial execution time (script measured): {serial_time:.4f} seconds")
    if mpi_output is not None:
        print(f"MPI execution time (script measured):    {mpi_time:.4f} seconds")

    # Run Comparator (optional)
    if os.path.exists(COMPARATOR_EXE_PATH):
        print("\n--- Running Comparison ---")
        comparator_cmd = [COMPARATOR_EXE_PATH, os.path.join(PROJECT_DIR, OUTPUT_SERIAL_FILE), os.path.join(PROJECT_DIR, OUTPUT_MPI_FILE)]
        run_program(comparator_cmd, "Comparator Program")
    else:
        print("\nComparator executable not found, skipping comparison.")

    print("\n--- Script Finished ---")