# Solving-2D-Heat-Equation-using-MPI
Parallel and Distributed Programming Project 

## Run Guide

1. **Compile the Code**

    ```bash
    mpic++ -o 2D_HeatEquation_MPI.cpp
    ```

2. **Run with MPI**
    Make sure the heat_equation_2d_mpi file is executable. In your terminal run:

    ```bash
    chmod +x ./heat_equation_2d_mpi
    ```

    Replace `<num_processes>` with the number of processes you want to use:

    ```bash
    mpiexec -np <num_processes> ./heat_equation_2d_mpi
    ```
3. **Run benchmark**
    To run the benchmark, use one of the following commands depending on your script:

    ```bash
    bash benchmark.sh
    ```
    or
    ```bash
    python3 benchmark.py
    ```
    Remember to install subprocess
    ```bash
    pip install subprocess
    ```
5. **Notes**

    - Ensure MPI is installed (`mpic++`, `mpiexec`).
    - Adjust input parameters in the source code or as required.
