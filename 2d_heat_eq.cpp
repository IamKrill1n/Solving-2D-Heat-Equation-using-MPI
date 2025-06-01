#include <stdio.h>
#include <stdlib.h>
#include <string.h> // For memcpy
#include <math.h>   // For fabs, if needed for convergence checks (not used here)
#include <time.h>   // For timing
#include <fstream>  // For file output
#include <iostream> // For std::fixed, std::setprecision
#include <iomanip>  // For std::fixed, std::setprecision

// Structure to hold simulation parameters
struct SimParams {
    int n_inner;          // Number of INNER grid points
    int N_total_pts;      // Total grid points including boundaries (n_inner + 2)
    int max_iterations;   // Number of time steps
    double c_const;       // Heat constant
    double ds;            // Spatial step (delta_s)
    double dt;            // Time step (delta_t)
    double boundary_top;
    double boundary_bottom;
    double boundary_left;
    double boundary_right;
};

// Helper to allocate 2D array
double** allocate_2d_array(int rows, int cols) {
    double* data = (double*)malloc(rows * cols * sizeof(double));
    if (!data) return NULL;
    double** array = (double**)malloc(rows * sizeof(double*));
    if (!array) {
        free(data);
        return NULL;
    }
    for (int i = 0; i < rows; i++) {
        array[i] = &(data[i * cols]);
    }
    return array;
}

void free_2d_array(double** array) {
    if (array) {
        if (array[0]) free(array[0]); // Free the contiguous block
        free(array); // Free the row pointers
    }
}

// Function to print a small section of the grid for debugging
void print_grid_section(double** grid, int N_total_pts, const char* title) {
    printf("\n--- %s (showing up to 10x10 or full if smaller) ---\n", title);
    int print_rows = N_total_pts < 10 ? N_total_pts : 10;
    int print_cols = N_total_pts < 10 ? N_total_pts : 10;
    for (int i = 0; i < print_rows; ++i) {
        for (int j = 0; j < print_cols; ++j) {
            printf("%6.2f ", grid[i][j]);
        }
        printf("\n");
    }
    printf("---------------------------------------------------\n");
}

void parse_arguments(int argc, char* argv[], SimParams& params) {
    if (argc > 1) params.n_inner = atoi(argv[1]);
    if (argc > 2) params.max_iterations = atoi(argv[2]);
    if (argc > 3) params.boundary_top = atof(argv[3]);
    if (argc > 4) params.boundary_bottom = atof(argv[4]);
    if (argc > 5) params.boundary_left = atof(argv[5]);
    if (argc > 6) params.boundary_right = atof(argv[6]);
}

void setup_simulation_parameters(SimParams& params) {
    params.N_total_pts = params.n_inner + 2;
    params.ds = 1.0 / (params.n_inner + 1); // If n_inner inner points, n_inner+1 intervals
    params.dt = (params.ds * params.ds) / (4.0 * params.c_const); // Stability condition

    // printf("Running 2D Heat Equation (Single Processor)\n");
    // printf("Grid: %dx%d total points (%dx%d inner points)\n", params.N_total_pts, params.N_total_pts, params.n_inner, params.n_inner);
    // printf("Iterations: %d\n", params.max_iterations);
    // printf("c = %.2f, ds = %.4f, dt = %.6f\n", params.c_const, params.ds, params.dt);
    // printf("Boundaries: T=%.1f, B=%.1f, L=%.1f, R=%.1f\n", params.boundary_top, params.boundary_bottom, params.boundary_left, params.boundary_right);
}

void initialize_grid(double** u_old, double** u_new, const SimParams& params) {
    for (int i = 0; i < params.N_total_pts; ++i) {
        for (int j = 0; j < params.N_total_pts; ++j) {
            if (i == 0) { // Top boundary
                u_old[i][j] = params.boundary_top;
            } else if (i == params.N_total_pts - 1) { // Bottom boundary
                u_old[i][j] = params.boundary_bottom;
            } else if (j == 0) { // Left boundary
                u_old[i][j] = params.boundary_left;
            } else if (j == params.N_total_pts - 1) { // Right boundary
                u_old[i][j] = params.boundary_right;
            } else { // Inner points
                u_old[i][j] = 0.0; // Or some other initial condition for inner points
            }
            u_new[i][j] = u_old[i][j]; // Initialize u_new with the same
        }
    }
}

void run_simulation(double** u_old, double** u_new, const SimParams& params) {
    for (int iter = 0; iter < params.max_iterations; ++iter) {
        // Compute u_new based on u_old for interior points
        for (int i = 1; i < params.N_total_pts - 1; ++i) {
            for (int j = 1; j < params.N_total_pts - 1; ++j) {
                u_new[i][j] = u_old[i][j] +
                    params.c_const * params.dt / (params.ds * params.ds) *
                    (u_old[i + 1][j] + u_old[i - 1][j] +
                     u_old[i][j + 1] + u_old[i][j - 1] -
                     4.0 * u_old[i][j]);
            }
        }

        // Update u_old = u_new
        if (iter < params.max_iterations - 1) { 
            for (int i_row = 0; i_row < params.N_total_pts; ++i_row) {
                memcpy(u_old[i_row], u_new[i_row], params.N_total_pts * sizeof(double));
            }
        }
    }
}

void write_grid_to_file(double** grid, const SimParams& params, const char* filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        fprintf(stderr, "Error opening file %s for writing.\n", filename);
        return;
    }
    outfile << params.N_total_pts << " " << params.N_total_pts << std::endl;
    for (int i = 0; i < params.N_total_pts; ++i) {
        for (int j = 0; j < params.N_total_pts; ++j) {
            outfile << grid[i][j] << (j == params.N_total_pts - 1 ? "" : " ");
        }
        outfile << std::endl;
    }
    outfile.close();
    printf("Final grid written to %s\n", filename);
}

void print_final_results(double** u_final, const SimParams& params, double time_spent) {
    // printf("\nFinished %d iterations.\n", params.max_iterations);
    // printf("Time taken: %f seconds.\n", time_spent);
    std::cout << std::fixed << std::setprecision(6) << time_spent << std::endl;

    // if (params.N_total_pts <= 20) {
    //     print_grid_section(u_final, params.N_total_pts, "Final u_new (result)");
    // }

    // write_grid_to_file(u_final, params, "output_serial.txt"); // Commented out
}

int main(int argc, char* argv[]) {
    SimParams params;
    params.n_inner = 100;       // Default number of INNER grid points
    params.max_iterations = 10000; // Default number of time steps
    params.c_const = 0.1;        // Default heat constant
    // Default boundary values, will be overwritten by parse_arguments if provided
    params.boundary_top = 10.0;
    params.boundary_bottom = 40.0;
    params.boundary_left = 20.0;
    params.boundary_right = 30.0;

    parse_arguments(argc, argv, params);
    setup_simulation_parameters(params);

    double** u_old = allocate_2d_array(params.N_total_pts, params.N_total_pts);
    double** u_new = allocate_2d_array(params.N_total_pts, params.N_total_pts);

    if (!u_old || !u_new) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return 1;
    }

    initialize_grid(u_old, u_new, params);

    // if (params.N_total_pts <= 20) {
    //     print_grid_section(u_old, params.N_total_pts, "Initial u_old");
    // }

    clock_t start_time = clock();
    run_simulation(u_old, u_new, params);
    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    print_final_results(u_new, params, time_spent); // u_new contains the final state

    free_2d_array(u_old);
    free_2d_array(u_new);
    return 0;
}