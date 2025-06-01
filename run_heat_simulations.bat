@echo off
setlocal

REM --- Configuration ---
set CPP_SERIAL_SRC=2d_heat_eq.cpp
set CPP_SERIAL_EXE=2d_heat_eq.exe
set CPP_MPI_SRC=2D_HeatEquation_MPI.cpp
set CPP_MPI_EXE=2D_HeatEquation_MPI.exe

REM --- Compiler (adjust if necessary) ---
set COMPILER_SERIAL=g++
set COMPILER_MPI=mpic++
set CXXFLAGS=-O2 -Wall

REM --- Get Parameters from User ---
echo =====================================================
echo      2D Heat Equation Simulation Runner
echo =====================================================
:get_params
set /p "N_INNER=Enter number of inner grid points (e.g., 100): "
set /p "MAX_ITER=Enter number of max iterations (e.g., 1000): "

if not defined N_INNER ( echo Inner grid points not set. Exiting. & goto :eof )
if not defined MAX_ITER ( echo Max iterations not set. Exiting. & goto :eof )

echo.
echo --- Parameters Set ---
echo Inner Grid Points (n_inner): %N_INNER%
echo Max Iterations: %MAX_ITER%
echo.

REM --- Compile Serial Version (if needed) ---
echo --- Compiling Serial Version (%CPP_SERIAL_SRC%) ---
if not exist "%CPP_SERIAL_EXE%" (
    echo %CPP_SERIAL_EXE% not found. Compiling...
    %COMPILER_SERIAL% %CPP_SERIAL_SRC% -o %CPP_SERIAL_EXE% %CXXFLAGS%
    if errorlevel 1 ( echo Compilation FAILED for %CPP_SERIAL_SRC%. Exiting. & goto :eof )
    echo Compilation successful.
) else (
    echo %CPP_SERIAL_EXE% found. Checking if source is newer...
    for /f %%a in ('powershell -Command "(Get-Item '%CPP_SERIAL_SRC%').LastWriteTime -gt (Get-Item '%CPP_SERIAL_EXE%').LastWriteTime"') do (
        if "%%a"=="True" (
            echo Source file %CPP_SERIAL_SRC% is newer. Recompiling...
            %COMPILER_SERIAL% %CPP_SERIAL_SRC% -o %CPP_SERIAL_EXE% %CXXFLAGS%
            if errorlevel 1 ( echo Recompilation FAILED for %CPP_SERIAL_SRC%. Exiting. & goto :eof )
            echo Recompilation successful.
        ) else (
            echo Executable is up-to-date.
        )
    )
)
echo.

REM --- Run Serial Version ---
echo --- Running Serial Version (%CPP_SERIAL_EXE%) ---
REM The C++ code prints time. We'll capture that.
REM Using PowerShell's Measure-Command for more robust timing if needed, but relying on program's output for now.
call .\%CPP_SERIAL_EXE% %N_INNER% %MAX_ITER% > temp_serial_output.txt
REM Extract time from the output file (assumes "Time taken: X.XXXXXX seconds.")
for /f "tokens=3 delims=: " %%t in ('findstr /C:"Time taken:" temp_serial_output.txt') do (
    set SERIAL_TIME=%%t
)
if defined SERIAL_TIME (
    echo Serial Execution Time: %SERIAL_TIME% seconds.
) else (
    echo Could not parse serial execution time from output.
)
findstr /V /C:"Time taken:" temp_serial_output.txt
if exist temp_serial_output.txt del temp_serial_output.txt
echo.

REM --- Compile MPI Version (if needed) ---
echo --- Compiling MPI Version (%CPP_MPI_SRC%) ---
if not exist "%CPP_MPI_EXE%" (
    echo %CPP_MPI_EXE% not found. Compiling...
    %COMPILER_MPI% %CPP_MPI_SRC% -o %CPP_MPI_EXE% %CXXFLAGS%
    if errorlevel 1 ( echo Compilation FAILED for %CPP_MPI_SRC%. Exiting. & goto :eof )
    echo Compilation successful.
) else (
    echo %CPP_MPI_EXE% found. Checking if source is newer...
    for /f %%a in ('powershell -Command "(Get-Item '%CPP_MPI_SRC%').LastWriteTime -gt (Get-Item '%CPP_MPI_EXE%').LastWriteTime"') do (
        if "%%a"=="True" (
            echo Source file %CPP_MPI_SRC% is newer. Recompiling...
            %COMPILER_MPI% %CPP_MPI_SRC% -o %CPP_MPI_EXE% %CXXFLAGS%
            if errorlevel 1 ( echo Recompilation FAILED for %CPP_MPI_SRC%. Exiting. & goto :eof )
            echo Recompilation successful.
        ) else (
            echo Executable is up-to-date.
        )
    )
)
echo.

REM --- Get MPI Processes ---
:get_mpi_procs
set /p "NUM_PROCS=Enter number of MPI processes (e.g., 4): "
if not defined NUM_PROCS ( echo Number of MPI processes not set. Exiting. & goto :eof )
echo Number of MPI Processes: %NUM_PROCS%
echo.

REM --- Run MPI Version ---
echo --- Running MPI Version (%CPP_MPI_EXE% with %NUM_PROCS% processes) ---
REM The C++ code prints time from rank 0. We'll capture that.
mpiexec -n %NUM_PROCS% .\%CPP_MPI_EXE% %N_INNER% %MAX_ITER% > temp_mpi_output.txt
REM Extract time from the output file (assumes "... in X.XXXXXX seconds using ...")
for /f "tokens=8 delims= " %%t in ('findstr /C:"seconds using" temp_mpi_output.txt') do (
    set MPI_TIME=%%t
)
if defined MPI_TIME (
    echo MPI Execution Time: %MPI_TIME% seconds.
) else (
    echo Could not parse MPI execution time from output.
)
findstr /V /C:"seconds using" temp_mpi_output.txt
if exist temp_mpi_output.txt del temp_mpi_output.txt
echo.

REM --- (Optional) Run Comparison ---
set /p "RUN_COMPARE=Do you want to run the output comparison (compare_outputs.exe)? (y/n): "
if /i "%RUN_COMPARE%"=="y" (
    if exist compare_outputs.exe (
        echo.
        echo --- Running Output Comparison (compare_outputs.exe) ---
        .\compare_outputs.exe
    ) else (
        echo compare_outputs.exe not found. Skipping comparison.
    )
)

echo.
echo =====================================================
echo                  All Done!
echo =====================================================

endlocal
goto :eof

:error_exit
echo An error occurred. Exiting.
endlocal