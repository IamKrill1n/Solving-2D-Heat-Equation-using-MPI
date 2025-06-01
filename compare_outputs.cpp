#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>   // For fabs
#include <iomanip> // For std::setprecision

// Helper to read a grid from a file
std::vector<std::vector<double>> read_grid_from_file(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return {};
    }

    int rows, cols;
    infile >> rows >> cols;
    if (rows <= 0 || cols <= 0) {
        std::cerr << "Invalid dimensions in file: " << filename << std::endl;
        return {};
    }

    std::vector<std::vector<double>> grid(rows, std::vector<double>(cols));
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (!(infile >> grid[i][j])) {
                std::cerr << "Error reading data from file: " << filename << " at " << i << "," << j << std::endl;
                return {};
            }
        }
    }
    infile.close();
    return grid;
}

int main() {
    std::string serial_file = "output_serial.txt";
    std::string mpi_file = "output_mpi.txt";

    std::vector<std::vector<double>> grid_serial = read_grid_from_file(serial_file);
    std::vector<std::vector<double>> grid_mpi = read_grid_from_file(mpi_file);

    if (grid_serial.empty() || grid_mpi.empty()) {
        std::cerr << "Failed to read one or both grid files. Exiting." << std::endl;
        return 1;
    }

    if (grid_serial.size() != grid_mpi.size() || 
        (grid_serial.size() > 0 && grid_serial[0].size() != grid_mpi[0].size())) {
        std::cerr << "Grid dimensions do not match between " << serial_file << " and " << mpi_file << std::endl;
        std::cerr << "Serial: " << grid_serial.size() << "x" << (grid_serial.empty() ? 0 : grid_serial[0].size()) << std::endl;
        std::cerr << "MPI:    " << grid_mpi.size() << "x" << (grid_mpi.empty() ? 0 : grid_mpi[0].size()) << std::endl;
        return 1;
    }

    double max_diff = 0.0;
    double sum_sq_diff = 0.0;
    int diff_count = 0;
    double tolerance = 1e-5; // Define a suitable tolerance

    int rows = grid_serial.size();
    int cols = grid_serial[0].size();

    std::cout << std::fixed << std::setprecision(8);

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            double diff = std::fabs(grid_serial[i][j] - grid_mpi[i][j]);
            if (diff > max_diff) {
                max_diff = diff;
            }
            sum_sq_diff += diff * diff;
            if (diff > tolerance) {
                diff_count++;
                // Optionally print differing values for debugging
                // std::cout << "Difference at (" << i << "," << j << "): Serial=" << grid_serial[i][j]
                //           << ", MPI=" << grid_mpi[i][j] << ", Diff=" << diff << std::endl;
            }
        }
    }

    double mse = (rows * cols > 0) ? sum_sq_diff / (rows * cols) : 0.0;
    double rmse = std::sqrt(mse);

    std::cout << "Comparison Results:" << std::endl;
    std::cout << "-------------------" << std::endl;
    std::cout << "Max absolute difference: " << max_diff << std::endl;
    std::cout << "Mean Squared Error (MSE): " << mse << std::endl;
    std::cout << "Root Mean Squared Error (RMSE): " << rmse << std::endl;
    std::cout << "Number of values differing by more than tolerance (" << tolerance << "): " << diff_count << std::endl;

    if (diff_count == 0 && max_diff <= tolerance) {
        std::cout << "\nOutputs are considered close enough." << std::endl;
    } else {
        std::cout << "\nOutputs have significant differences." << std::endl;
    }

    return 0;
}