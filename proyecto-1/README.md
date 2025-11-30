# Paint App

A simple paint application built with Rust. Made for Graphics Introduction at Central University of Venezuela by Rafael E. Contreras :) 

## How to Run

### Windows

This project includes a PowerShell script (`build.ps1`) that automates the entire setup and build process.

1.  **Open PowerShell**: Navigate to the project's root directory.
2.  **Execution Policy (If you have an error)**: If you get an error about script execution being disabled, run the following command to allow the script to run for the current process, then try step 2 again:
    ```powershell
    Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
    ```
3.  **Run the Build Script**: Execute the following command:
    ```powershell
    .\build.ps1
    ```
4.  Restore the default execution policy. If the default was another one, change it to that one.
    ```powershell
    Set-ExecutionPolicy Restricted
    ```
5.  **Run the App**: After the build is complete, the executable will be located at `target\release\paint_app.exe`. You can run it with:
    ```powershell
    .\target\release\paint_app.exe
    ```

### Linux

1.  **Install Rust**: If you don't have Rust installed, you can install it with:
    ```bash
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
    ```
2.  **Build and Run**: Navigate to the project's root directory and run the following commands:
    ```bash
    source prepare.sh
    cargo run --release
    ```
