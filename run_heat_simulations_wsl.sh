#!/bin/bash

# --- Configuration ---
SERIAL_SRC="2d_heat_eq.cpp"
SERIAL_EXE="./2d_heat_eq.exe"
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
# $1: min value, $2: max value
random_float() {
    local min=$1
    local max=$2
    # $RANDOM returns an integer between 0 and 32767.
    echo "$(awk -v min="$min" -v max="$max" -v r="$RANDOM" 'BEGIN { print min + (r/32768.0)*(max - min) }')"
}

# --- Main Script ---
echo "--- 2D Heat Equation Simulation Runner (WSL Bash with Random Boundaries) ---"

# Get parameters from user
read -p "Enter N_INNER (e.g., 100, default: 100): " n_inner_input
N_INNER=${n_inner_input:-100}

read -p "Enter MAX_ITERATIONS (e.g., 1000, default: 1000): " max_iter_input
MAX_ITERATIONS=${max_iter_input:-1000}

read -p "Enter number of MPI processes (e.g., 4, default: 4): " num_procs_input
NUM_PROCS=${num_procs_input:-4}

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
CXXFLAGS_COMMON="-O3 -Wall -std=c++17//// filepath: d:\exercise\parallel\project\run_heat_simulations_wsl.sh
# Use bash’s $RANDOM to produce a random float between min and max.
random_float() {
    local min=$1
    local max=$2
    # $RANDOM returns an integer between 0 and 32767.
    echo "$(awk -v min="$min" -v max="$max" -v r="$RANDOM" 'BEGIN { print min + (r/32768.0)*(max - min) }')"
}" # Added -std=c++11 for iostream features if needed

echo "\n--- Compilation Check ---"
compile_if_needed "$SERIAL_SRC" "$SERIAL_EXE" "g++" "$CXXFLAGS_COMMON"
compile_if_needed "$MPI_SRC" "$MPI_EXE" "mpicxx" "$CXXFLAGS_COMMON"

echo "\n--- Running Simulations & Recording Program-Reported Times ---"

# Run Serial Program and capture its single line time output
echo "Running Serial Program ($SERIAL_EXE $N_INNER $MAX_ITERATIONS $BOUNDARY_TOP $BOUNDARY_BOTTOM $BOUNDARY_LEFT $BOUNDARY_RIGHT)..."
SERIAL_TIME_REPORTED=$($SERIAL_EXE $N_INNER $MAX_ITERATIONS $BOUNDARY_TOP $BOUNDARY_BOTTOM $BOUNDARY_LEFT $BOUNDARY_RIGHT)

if [ -n "$SERIAL_TIME_REPORTED" ]; then
    echo "Serial Program Reported Execution Time: $SERIAL_TIME_REPORTED s"
else
    echo "Could not get execution time from Serial Program output."
fi

# Run MPI Program and capture its single line time output
echo "\nRunning MPI Program (mpiexec -n $NUM_PROCS $MPI_EXE $N_INNER $MAX_ITERATIONS $BOUNDARY_TOP $BOUNDARY_BOTTOM $BOUNDARY_LEFT $BOUNDARY_RIGHT)..."
MPI_TIME_REPORTED=$(mpiexec -n $NUM_PROCS $MPI_EXE $N_INNER $MAX_ITERATIONS $BOUNDARY_TOP $BOUNDARY_BOTTOM $BOUNDARY_LEFT $BOUNDARY_RIGHT)

if [ -n "$MPI_TIME_REPORTED" ]; then
    # MPI output might have extra newlines from different ranks, ensure we get the one from rank 0
    # Assuming rank 0 prints last or its output is what we care about for the single time value.
    # If multiple ranks print the time, this will take the first line of output from mpiexec.
    # If only rank 0 prints, this will be its output.
    CLEANED_MPI_TIME=$(echo "$MPI_TIME_REPORTED" | awk 'NF{print $1; exit}') # Get first non-empty line, first field
    echo "MPI Program Reported Execution Time: $CLEANED_MPI_TIME s"
else
    echo "Could not get execution time from MPI Program output."
fi

echo "\n--- Script Finished ---"