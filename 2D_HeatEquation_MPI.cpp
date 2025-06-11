#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include <fstream>  
#include <vector>   
#include <iostream>
#include <iomanip> 

struct SimParamsMPI {
    int n_global;         // Number of INNER grid points globally
    int N_total_pts;      // Total grid points including boundaries (n_global + 2)
    int max_iterations;   // Number of time steps
    double c_const;       // Heat constant
    double ds;            // Spatial step (delta_s)
    double dt;            // Time step (delta_t)
    double boundary_top;
    double boundary_bottom;
    double boundary_left;
    double boundary_right;
    int rank;             // MPI rank of the current process
    int size;             // Total number of MPI processes
    int my_num_rows;      // Number of actual rows this process handles
    int my_start_row_global; // Global starting row index for this process
    int ifirst_comp_local; // First local row index to compute
    int ilast_comp_local;  // Last local row index to compute
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

void parse_mpi_arguments(int argc, char* argv[], SimParamsMPI& params) {
    if (argc > 1) params.n_global = atoi(argv[1]);
    if (argc > 2) params.max_iterations = atoi(argv[2]);
    if (argc > 3) params.boundary_top = atof(argv[3]);
    if (argc > 4) params.boundary_bottom = atof(argv[4]);
    if (argc > 5) params.boundary_left = atof(argv[5]);
    if (argc > 6) params.boundary_right = atof(argv[6]);
}

void setup_mpi_simulation_parameters(SimParamsMPI& params) {
    params.N_total_pts = params.n_global + 2;
    params.ds = 1.0 / (params.n_global + 1);
    params.dt = (params.ds * params.ds) / (4.0 * params.c_const);

    //Domain Decomposition
    int rows_per_proc = params.N_total_pts / params.size;
    int remainder_rows = params.N_total_pts % params.size;
    params.my_num_rows = rows_per_proc + (params.rank < remainder_rows ? 1 : 0);
    params.my_start_row_global = params.rank * rows_per_proc + (params.rank < remainder_rows ? params.rank : remainder_rows);

    //Determine computation range (ifirst, ilast) for local rows
    params.ifirst_comp_local = 1;
    params.ilast_comp_local = params.my_num_rows;

    if (params.rank == 0) {
        params.ifirst_comp_local = 2; 
    }
    if (params.rank == params.size - 1) {
        params.ilast_comp_local = params.my_num_rows - 1;
    }
    
    if (params.ifirst_comp_local > params.ilast_comp_local) {
        params.ifirst_comp_local = 1; 
        params.ilast_comp_local = 0;  
    }

    // if (params.rank == 0) {
    //     printf("Running 2D Heat Equation (MPI Parallel)");
    //     printf("Global Grid: %dx%d total points (%dx%d inner points)\n", params.N_total_pts, params.N_total_pts, params.n_global, params.n_global);
    //     printf("Iterations: %d\n", params.max_iterations);
    //     printf("c = %.2f, ds = %.4f, dt = %.6f\n", params.c_const, params.ds, params.dt);
    //     printf("MPI Processes: %d\n", params.size);
    //     printf("Boundaries: T=%.1f, B=%.1f, L=%.1f, R=%.1f\n", params.boundary_top, params.boundary_bottom, params.boundary_left, params.boundary_right);
    // }
}

void initialize_local_grid(double** u_old_local, double** u_new_local, const SimParamsMPI& params) {
    for (int i_local = 0; i_local < params.my_num_rows + 2; ++i_local) {
        for (int j_col = 0; j_col < params.N_total_pts; ++j_col) {
            u_old_local[i_local][j_col] = 0.0;
            u_new_local[i_local][j_col] = 0.0;
        }
    }

    for (int i_local_actual = 1; i_local_actual <= params.my_num_rows; ++i_local_actual) {
        int i_global = params.my_start_row_global + i_local_actual - 1;
        for (int j_col = 0; j_col < params.N_total_pts; ++j_col) {
            if (i_global == 0) {
                u_old_local[i_local_actual][j_col] = params.boundary_top;
            } else if (i_global == params.N_total_pts - 1) {
                u_old_local[i_local_actual][j_col] = params.boundary_bottom;
            } else if (j_col == 0) {
                u_old_local[i_local_actual][j_col] = params.boundary_left;
            } else if (j_col == params.N_total_pts - 1) {
                u_old_local[i_local_actual][j_col] = params.boundary_right;
            } else {
                u_old_local[i_local_actual][j_col] = 0.0; // f(x, y)
            }
            u_new_local[i_local_actual][j_col] = u_old_local[i_local_actual][j_col];
        }
    }
}

void perform_computation_step(double** u_old_local, double** u_new_local, const SimParamsMPI& params) {
    for (int i_local = params.ifirst_comp_local; i_local <= params.ilast_comp_local; ++i_local) {
        for (int j_col = 1; j_col < params.N_total_pts - 1; ++j_col) {
            u_new_local[i_local][j_col] = u_old_local[i_local][j_col] +
                params.c_const * params.dt / (params.ds * params.ds) *
                (u_old_local[i_local + 1][j_col] + u_old_local[i_local - 1][j_col] +
                 u_old_local[i_local][j_col + 1] + u_old_local[i_local][j_col - 1] -
                 4.0 * u_old_local[i_local][j_col]);
        }
    }
}

void exchange_ghost_rows(double** u_new_local, const SimParamsMPI& params) {
    MPI_Request reqs[4];
    MPI_Status stats[4];
    int req_count = 0;

    if (params.rank > 0) {
        MPI_Isend(u_new_local[1], params.N_total_pts, MPI_DOUBLE,
                  params.rank - 1, 0, MPI_COMM_WORLD, &reqs[req_count++]);
        MPI_Irecv(u_new_local[0], params.N_total_pts, MPI_DOUBLE,
                  params.rank - 1, 1, MPI_COMM_WORLD, &reqs[req_count++]);
    }

    if (params.rank < params.size - 1) {
        MPI_Isend(u_new_local[params.my_num_rows], params.N_total_pts, MPI_DOUBLE,
                  params.rank + 1, 1, MPI_COMM_WORLD, &reqs[req_count++]);
        MPI_Irecv(u_new_local[params.my_num_rows + 1], params.N_total_pts, MPI_DOUBLE,
                  params.rank + 1, 0, MPI_COMM_WORLD, &reqs[req_count++]);
    }
    
    if(req_count > 0) {
        MPI_Waitall(req_count, reqs, stats);
    }
}

void update_old_local_grid(double** u_old_local, double** u_new_local, const SimParamsMPI& params) {
    for (int i_local = 0; i_local < params.my_num_rows + 2; ++i_local) {
        memcpy(u_old_local[i_local], u_new_local[i_local], params.N_total_pts * sizeof(double));
    }
}

void run_mpi_simulation(double** u_old_local, double** u_new_local, const SimParamsMPI& params) {
    for (int iter = 0; iter < params.max_iterations; ++iter) {
        perform_computation_step(u_old_local, u_new_local, params);
        exchange_ghost_rows(u_new_local, params);
        update_old_local_grid(u_old_local, u_new_local, params);
    }
}

void gather_and_write_grid_to_file(double** u_local_final, const SimParamsMPI& params, const char* filename) {
    double** global_grid = nullptr;
    std::vector<int> recvcounts;
    std::vector<int> displs;

    if (params.rank == 0) {
        global_grid = allocate_2d_array(params.N_total_pts, params.N_total_pts);
        if (!global_grid) {
            fprintf(stderr, "Rank 0: Failed to allocate memory for global grid.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        recvcounts.resize(params.size);
        displs.resize(params.size);
        int current_displacement = 0;

        for (int r = 0; r < params.size; ++r) {
            int rows_on_proc_r = (params.N_total_pts / params.size) + (r < (params.N_total_pts % params.size) ? 1 : 0);
            // We are gathering actual data rows, not ghost cells.
            // Each row has N_total_pts columns.
            recvcounts[r] = rows_on_proc_r * params.N_total_pts;
            displs[r] = current_displacement;
            current_displacement += recvcounts[r];
        }
    }

    // Each process sends its actual data rows (from u_local_final[1] to u_local_final[params.my_num_rows])
    // The data is contiguous in memory for these rows if allocate_2d_array stores it that way.
    // We send params.my_num_rows * params.N_total_pts elements.
    MPI_Gatherv(&u_local_final[1][0], params.my_num_rows * params.N_total_pts, MPI_DOUBLE,
                global_grid ? &global_grid[0][0] : NULL, // Only rank 0 provides a valid receive buffer
                recvcounts.data(), displs.data(), MPI_DOUBLE,
                0, MPI_COMM_WORLD);

    if (params.rank == 0) {
        std::ofstream outfile(filename);
        if (!outfile.is_open()) {
            fprintf(stderr, "Rank 0: Error opening file %s for writing.\n", filename);
        } else {
            outfile << params.N_total_pts << " " << params.N_total_pts << std::endl;
            for (int i = 0; i < params.N_total_pts; ++i) {
                for (int j = 0; j < params.N_total_pts; ++j) {
                    outfile << global_grid[i][j] << (j == params.N_total_pts - 1 ? "" : " ");
                }
                outfile << std::endl;
            }
            outfile.close();
            printf("Rank 0: Final grid written to %s\n", filename);
        }
        free_2d_array(global_grid);
    }
}


int main(int argc, char* argv[]) {
    SimParamsMPI params;
    params.n_global = 1000;       // Default global inner grid points
    params.max_iterations = 1000; // Default iterations
    params.c_const = 0.1;         // Default heat constant
    // Initialize boundary values with defaults
    params.boundary_top = 10.0;    // Default top boundary
    params.boundary_bottom = 40.0; // Default bottom boundary
    params.boundary_left = 20.0;   // Default left boundary
    params.boundary_right = 30.0;  // Default right boundary

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &params.rank);
    MPI_Comm_size(MPI_COMM_WORLD, &params.size);

    parse_mpi_arguments(argc, argv, params);
    setup_mpi_simulation_parameters(params);

    double** u_old_local = allocate_2d_array(params.my_num_rows + 2, params.N_total_pts);
    double** u_new_local = allocate_2d_array(params.my_num_rows + 2, params.N_total_pts);

    if (!u_old_local || !u_new_local) {
        fprintf(stderr, "Rank %d: Failed to allocate memory.\n", params.rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    initialize_local_grid(u_old_local, u_new_local, params);

    double start_time, end_time;
    MPI_Barrier(MPI_COMM_WORLD);
    start_time = MPI_Wtime();

    run_mpi_simulation(u_old_local, u_new_local, params);

    MPI_Barrier(MPI_COMM_WORLD);
    end_time = MPI_Wtime();

    gather_and_write_grid_to_file(u_new_local, params, "output_mpi.txt"); // Commented out

    if (params.rank == 0) {
        printf("Finished %d iterations for %dx%d grid (%d inner) in %f seconds using %d processes.\n",
               params.max_iterations, params.N_total_pts, params.N_total_pts, params.n_global, end_time - start_time, params.size);
        printf("Parameters: c=%.2f, ds=%.4f, dt=%.6f\n", params.c_const, params.ds, params.dt);
        std::cout << std::fixed << std::setprecision(6) << (end_time - start_time) << std::endl;
    }

    free_2d_array(u_old_local);
    free_2d_array(u_new_local);
    MPI_Finalize();
    return 0;
}