import os
import subprocess


def main():
    datasets_dir = "../datasets"
    output_dir = "../experiment/output_reports"

    # Ensure output folder exists
    os.makedirs(output_dir, exist_ok=True)

    # Files to measure after running each pipeline
    target_files = [
        "compressed.gcis_c.C",
        "compressed.gcis_c.R",
        "grammar.gcis_g",
    ]

    total_bytes = 0

    # Ensure the datasets directory exists
    if not os.path.exists(datasets_dir):
        print(f"Directory not found: {datasets_dir}")
        return

    # Recursively traverse all folders and subfolders
    for root, _, files in os.walk(datasets_dir):
        for filename in files:
            dataset_path = os.path.join(root, filename)

            # Construct command chain using the dataset path
            command = (
                f".././build/gcis < '{dataset_path}' && "
                f".././repair/irepair compressed.gcis_c && "
                f".././repair/idespair compressed.gcis_c"
            )

            try:
                # Run commands and capture both stdout and stderr
                result = subprocess.run(
                    command,
                    shell=True,
                    check=True,
                    capture_output=True,
                    text=True,
                )

                # Calculate individual encoding size for this specific dataset run
                run_bytes = 0
                size_breakdown = []
                for file_path in target_files:
                    if os.path.exists(file_path):
                        file_size = os.path.getsize(file_path)
                        run_bytes += file_size
                        size_breakdown.append(f"  - {file_path}: {file_size:,} bytes")
                    else:
                        size_breakdown.append(f"  - {file_path}: File not found")

                # Accumulate grand total
                total_bytes += run_bytes

                # Save captured output AND encoding sizes to report file
                report_path = os.path.join(output_dir, f"{filename}.txt")
                with open(report_path, "w", encoding="utf-8") as report_file:
                    report_file.write(f"=== Output Report for {filename} ===\n\n")
                    report_file.write("--- ENCODING SIZES ---\n")
                    report_file.write("\n".join(size_breakdown))
                    report_file.write(f"\nTotal Run Encoding Size: {run_bytes:,} bytes\n\n")
                    report_file.write("--- STDOUT ---\n")
                    report_file.write(result.stdout)
                    if result.stderr:
                        report_file.write("\n--- STDERR ---\n")
                        report_file.write(result.stderr)

            except subprocess.CalledProcessError as e:
                print(f"Error processing {dataset_path}: {e}")
                # Save error logs if execution fails
                error_report_path = os.path.join(
                    output_dir, f"{filename}_error.txt"
                )
                with open(
                    error_report_path, "w", encoding="utf-8"
                ) as error_file:
                    error_file.write(
                        f"=== Error Report for {filename} ===\n\n"
                    )
                    error_file.write(e.stderr or str(e))


if __name__ == "__main__":
    main()