import pandas as pd
import matplotlib.pyplot as plt
import os

def generate_plots(benchmark_dir="benchmark"):
    """
    Reads all performance data from CSV files in a directory and generates a separate plot for each file.

    Args:
        benchmark_dir (str): The path to the directory containing benchmark CSV files.
    """
    try:
        csv_files = sorted([f for f in os.listdir(benchmark_dir) if f.endswith('.csv')])
        if not csv_files:
            print(f"No CSV files found in '{benchmark_dir}'.")
            return

        algo_colors = {'vanilla': 'blue', 'optimized': 'green'}

        for csv_file in csv_files:
            fig, ax = plt.subplots(figsize=(10, 6))
            
            base_name = os.path.splitext(csv_file)[0]
            file_path = os.path.join(benchmark_dir, csv_file)
            
            try:
                df = pd.read_csv(file_path)
            except Exception as e:
                print(f"Error reading {csv_file}: {e}")
                continue

            for algorithm, color in algo_colors.items():
                algo_df = df[df['algorithm'] == algorithm]
                if not algo_df.empty:
                    ax.plot(algo_df['num_ellipses'], algo_df['time'], color=color, label=algorithm)

            ax.legend(loc='upper left')
            ax.set_xlabel('Number of Ellipses')
            ax.set_ylabel('Time (seconds)')
            ax.set_title(f'Algorithm Performance for {base_name}')
            ax.grid(True)

            output_filename = os.path.join(benchmark_dir, f'{base_name}.png')
            plt.savefig(output_filename, bbox_inches='tight')
            print(f"Plot saved as {output_filename}")
            
            plt.close(fig) # Close the figure to free memory

    except FileNotFoundError:
        print(f"Error: The directory '{benchmark_dir}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    # To run this script:
    # 1. Make sure you have pandas and matplotlib installed:
    #    pip install pandas matplotlib
    # 2. Run the C++ program to generate benchmark CSVs in the 'benchmark' folder.
    # 3. Run this script:
    #    python plot_performance.py
    generate_plots()
