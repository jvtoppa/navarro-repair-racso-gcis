import os
import shutil
import subprocess


def main():
    datasets_dir = "../datasets"
    output_dir = "../experiment/output_reports"

    # Convert tool paths to absolute paths
    repair_bin = os.path.abspath("../repair/repair")
    despair_bin = os.path.abspath("../repair/despair")

    # Ensure output folder exists
    os.makedirs(output_dir, exist_ok=True)

    total_bytes = 0

    if not os.path.exists(datasets_dir):
        print(f"Directory not found: {datasets_dir}")
        return

    # Recursively traverse all folders
    for root, _, files in os.walk(datasets_dir):
        for filename in files:
            source_dataset_path = os.path.abspath(os.path.join(root, filename))

            # Path inside ../datasets/ root where repair will run
            local_target_path = os.path.abspath(os.path.join(datasets_dir, filename))

            # Skip copy if the file is already located directly in ../datasets/ root
            if source_dataset_path != local_target_path:
                shutil.copyfile(source_dataset_path, local_target_path)

            # Target files generated alongside the local target path
            target_files = [
                f"{local_target_path}.C",
                f"{local_target_path}.R",
            ]

            # Construct command sequence
            command = (
                f"'{repair_bin}' '{local_target_path}' && "
                f"'{despair_bin}' '{filename}'"
            )

            try:
                result = subprocess.run(
                    command,
                    shell=True,
                    check=True,
                    capture_output=True,
                    text=True,
                    cwd=datasets_dir,
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
                print(f"Error processing {source_dataset_path}: {e}")
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
                # Cleanup created target files (.C, .R)
                for tf in target_files:
                    if os.path.exists(tf):
                        os.remove(tf)

                # Remove the copied dataset from ../datasets/ root if it was copied from a subfolder
                if source_dataset_path != local_target_path and os.path.exists(local_target_path):
                    os.remove(local_target_path)

    print(f"Total Encoding size across all datasets: {total_bytes:,} bytes")


if __name__ == "__main__":
    main()