import subprocess
import os
import csv

MPI_SRC = "2D_HeatEquation_MPI.cpp"
MPI_EXE = "./heat_equation_2d_mpi"
CXXFLAGS_COMMON = "-O3 -Wall -std=c++17"
OUTPUT_CSV = "benchmark_results.csv"

def compile_if_needed(src_file, exe_file, compiler, flags):
    if not os.path.exists(exe_file) or os.path.getmtime(src_file) > os.path.getmtime(exe_file):
        print(f"Compiling {src_file}...")
        cmd = [compiler, src_file, "-o", exe_file] + flags.split()
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print("Compilation failed:")
            print(result.stderr)
            exit(1)

def run_simulation(n_inner, max_iter, num_procs, boundary_values, num_runs=3):
    total_time = 0.0
    cmd = [
        "mpiexec", "-n", str(num_procs), MPI_EXE,
        str(n_inner), str(max_iter),
        str(boundary_values[0]), str(boundary_values[1]),
        str(boundary_values[2]), str(boundary_values[3])
    ]
    for i in range(num_runs):
        result = subprocess.run(cmd, capture_output=True, text=True)
        try:
            time_str = result.stdout.strip().split()[0]
            total_time += float(time_str)
        except Exception:
            print(f"Failed to parse time (Run {i+1}):")
            print(result.stdout)
            return None
    return total_time / num_runs

def main():
    compile_if_needed(MPI_SRC, MPI_EXE, "mpic++", CXXFLAGS_COMMON)

    n_inner_list = [100, 200, 400]
    max_iter_list = [500, 1000]
    num_procs_list = [1, 2, 4, 8]

    # Boundary cố định, có thể chỉnh nếu muốn
    boundaries = [50.0, 50.0, 50.0, 50.0]

    with open(OUTPUT_CSV, "w", newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["N_INNER", "MAX_ITER", "NUM_PROCS", "ExecutionTime", "Boundary_Values"])

        for n_inner in n_inner_list:
            for max_iter in max_iter_list:
                for num_procs in num_procs_list:
                    avg_time = run_simulation(n_inner, max_iter, num_procs, boundaries)
                    if avg_time is not None:
                        # Format time với 6 chữ số thập phân
                        avg_time_str = f"{avg_time:.6f}"
                        writer.writerow([n_inner, max_iter, num_procs, avg_time_str, boundaries])
                        print(f"✅ Done: N={n_inner}, I={max_iter}, P={num_procs}, Time={avg_time_str}, Boundaries={boundaries}")

if __name__ == "__main__":
    main()
