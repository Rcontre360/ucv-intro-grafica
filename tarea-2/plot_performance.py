
import pandas as pd
import matplotlib.pyplot as plt

def plot_performance(csv_file):
    """
    Reads performance data from a CSV file and generates a plot.

    Args:
        csv_file (str): The path to the CSV file.
    """
    try:
        # Read the CSV file
        df = pd.read_csv(csv_file)

        # Separate data for each algorithm
        vanilla_df = df[df['algorithm'] == 'vanilla']
        optimized_df = df[df['algorithm'] == 'optimized']

        # Create the plot
        plt.figure(figsize=(10, 6))

        plt.plot(vanilla_df['num_ellipses'], vanilla_df['time'], label='Vanilla Algorithm')
        plt.plot(optimized_df['num_ellipses'], optimized_df['time'], label='Optimized Algorithm')

        # Add labels and title
        plt.xlabel('Number of Ellipses')
        plt.ylabel('Time (seconds)')
        plt.title('Algorithm Performance Comparison')
        plt.legend()
        plt.grid(True)

        # Save the plot
        plt.savefig('performance_graph.png')
        print("Plot saved as performance_graph.png")

    except FileNotFoundError:
        print(f"Error: The file '{csv_file}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    # To run this script:
    # 1. Make sure you have pandas and matplotlib installed:
    #    pip install pandas matplotlib
    # 2. Run the C++ program to generate benchmark.csv
    # 3. Run this script:
    #    python plot_performance.py
    plot_performance('benchmark.csv')
