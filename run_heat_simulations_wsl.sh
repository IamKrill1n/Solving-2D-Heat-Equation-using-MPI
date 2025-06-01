#!/bin/bash

# --- Configuration ---
MPI_SRC="2D_HeatEquation_MPI.cpp"
MPI_EXE="./heat_equation_2d_mpi"

# Function to compile C++ files
compile_if_needed() {
    local src_file=$1
    local exe_file=$2
    local compiler=$3
    local cxxflags=$4

    if [ ! -f "$exe_file" ] || [ "$src_file" -nt "$exe_file" ]; then
        echo "Compiling $src_file..."
        $compiler "$src_file" -o "$exe_file" $cxxflags
        if [ $? -eq 0 ]; then
            echo "Compilation of $src_file successful."
        else
            echo "Error compiling $src_file. Exiting."
            exit 1
        fi
    else
        echo "$exe_file is up to date."
    fi
}

# Function to generate a random float between min and max
random_float() {
    local min=$1
    local max=$2
    echo "$(awk -v min="$min" -v max="$max" -v r="$RANDOM" 'BEGIN { print min + (r/32768.0)*(max - min) }')"
}

echo "--- 2D Heat Equation Simulation Runner (WSL Bash with Random Boundaries) ---"

# Get parameters from user
read -p "Enter N_INNER (e.g., 100, default: 100): " n_inner_input
N_INNER=${n_inner_input:-100}

read -p "Enter MAX_ITERATIONS (e.g., 1000, default: 1000): " max_iter_input
MAX_ITERATIONS=${max_iter_input:-1000}

read -p "Enter number of MPI processes (e.g., 4, default: 4): " num_procs_input
NUM_PROCS=${num_procs_input:-4}

# Prompt for number of simulation runs
read -p "Enter number of simulation runs (default: 1): " runs_input
NUM_RUNS=${runs_input:-1}

# Generate random boundary values (0.0 to 100.0)
BOUNDARY_TOP=$(random_float 0 100)
BOUNDARY_BOTTOM=$(random_float 0 100)
BOUNDARY_LEFT=$(random_float 0 100)
BOUNDARY_RIGHT=$(random_float 0 100)

echo "Using Random Boundary Conditions:"
echo "  Top:    $BOUNDARY_TOP"
echo "  Bottom: $BOUNDARY_BOTTOM"
echo "  Left:   $BOUNDARY_LEFT"
echo "  Right:  $BOUNDARY_RIGHT"

# Define compiler flags (adjust as needed)
CXXFLAGS_COMMON="-O3 -Wall -std=c++17"

echo "\n--- Compilation Check ---"
compile_if_needed "$MPI_SRC" "$MPI_EXE" "mpic++" "$CXXFLAGS_COMMON"

echo "\n--- Running Simulations & Recording Program-Reported Times ---"

# Run MPI Program NUM_RUNS times and average its reported time
MPI_TOTAL=0
echo -e "\nRunning MPI Program (mpiexec -n $NUM_PROCS $MPI_EXE $N_INNER $MAX_ITERATIONS $BOUNDARY_TOP $BOUNDARY_BOTTOM $BOUNDARY_LEFT $BOUNDARY_RIGHT) for $NUM_RUNS runs..."
for (( i=1; i<=NUM_RUNS; i++ ))
do
    mpi_out=$(mpiexec -n $NUM_PROCS $MPI_EXE $N_INNER $MAX_ITERATIONS $BOUNDARY_TOP $BOUNDARY_BOTTOM $BOUNDARY_LEFT $BOUNDARY_RIGHT)
    # Clean output to take only the first non-empty line (assuming rank 0 prints the time)
    cleaned_out=$(echo "$mpi_out" | awk 'NF { print $1; exit }')
    echo "  Run $i: $cleaned_out"
    MPI_TOTAL=$(echo "$MPI_TOTAL + $cleaned_out" | bc -l)
done
MPI_AVG=$(echo "scale=6; $MPI_TOTAL / $NUM_RUNS" | bc -l)
echo "MPI Program Average Execution Time: $MPI_AVG"

echo -e "\n--- Script Finished ---"