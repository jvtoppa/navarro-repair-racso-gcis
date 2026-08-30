import os
import shutil
import subprocess


def main():
    datasets_dir = "../datasets"
    output_dir = "../experiment/output_reports"
    temp_dir = "./temp_workspace"

    # Convert tool paths to absolute paths to prevent status 127 errors
    repair_bin = os.path.abspath("../repair/repair")
    despair_bin = os.path.abspath("../repair/despair")

    # Ensure folders exist
    os.makedirs(output_dir, exist_ok=True)
    os.makedirs(temp_dir, exist_ok=True)

    total_bytes = 0

    if not os.path.exists(datasets_dir):
        print(f"Directory not found: {datasets_dir}")
        return

    # Recursively traverse all folders
    for root, _, files in os.walk(datasets_dir):
        for filename in files:
            dataset_path = os.path.abspath(os.path.join(root, filename))

            # Temporary file path where repair will write its output
            temp_file_path = os.path.abspath(os.path.join(temp_dir, filename))

            # Clear previous temp symlinks/files if existing
            if os.path.exists(temp_file_path) or os.path.islink(temp_file_path):
                os.remove(temp_file_path)

            # Create a symlink in the temporary folder
            os.symlink(dataset_path, temp_file_path)

            # Define expected output targets inside temp_dir
            target_files = [
                f"{temp_file_path}.C",
                f"{temp_file_path}.R",
            ]

            # Construct command using absolute tool paths
            command = (
                f"'{repair_bin}' '{temp_file_path}' && "
                f"'{despair_bin}' '{filename}'"
            )

            try:
                result = subprocess.run(
                    command,
                    shell=True,
                    check=True,
                    capture_output=True,
                    text=True,
                    cwd=temp_dir,
                )

                # Measure sizes
                run_bytes = 0
                size_breakdown = []
                for file_path in target_files:
                    if os.path.exists(file_path):
                        file_size = os.path.getsize(file_path)
                        run_bytes += file_size
                        size_breakdown.append(
                            f"  - {os.path.basename(file_path)}: {file_size:,} bytes"
                        )
                    else:
                        size_breakdown.append(
                            f"  - {os.path.basename(file_path)}: File not found"
                        )

                total_bytes += run_bytes

                # Save report
                report_path = os.path.join(output_dir, f"{filename}.txt")
                with open(report_path, "w", encoding="utf-8") as report_file:
                    report_file.write(
                        f"=== Output Report for {filename} ===\n\n"
                    )
                    report_file.write("--- ENCODING SIZES ---\n")
                    report_file.write("\n".join(size_breakdown))
                    report_file.write(
                        f"\nTotal Run Encoding Size: {run_bytes:,} bytes\n\n"
                    )
                    report_file.write("--- STDOUT ---\n")
                    report_file.write(result.stdout)
                    if result.stderr:
                        report_file.write("\n--- STDERR ---\n")
                        report_file.write(result.stderr)

            except subprocess.CalledProcessError as e:
                print(f"Error processing {dataset_path}: {e}")
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

            finally:
                # Cleanup generated temp files & symlinks after each run
                if os.path.exists(temp_file_path) or os.path.islink(
                    temp_file_path
                ):
                    os.remove(temp_file_path)
                for tf in target_files:
                    if os.path.exists(tf):
                        os.remove(tf)

    # Clean up temp workspace folder when done
    if os.path.exists(temp_dir):
        shutil.rmtree(temp_dir)

    print(f"Total Encoding size across all datasets: {total_bytes:,} bytes")


if __name__ == "__main__":
    main()