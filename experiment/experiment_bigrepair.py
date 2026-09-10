import os
import shutil
import subprocess

SKIP_SUFFIXES = (
    ".r",
    ".c.R",
    ".c.C",
    ".c",
    ".c.out",
    ".out",
    ".orig_backup",
    ".pre_repair_backup",
    ".log",
    ".c.log",
    ".c.pre_repair_backup",
    ".C",
    ".R",
)


def cleanup_generated_files(base_path: str):
    """Removes all generated files matching known artifact suffixes for a given file base."""
    for suffix in SKIP_SUFFIXES:
        target = f"{base_path}{suffix}"
        if os.path.exists(target):
            try:
                os.remove(target)
            except OSError:
                pass


def main():
    datasets_dir = "../datasets"
    output_dir = "../experiment/output_reports"

    # Absolute paths to tools
    bigrepair_bin = os.path.abspath("../bigrepair/bigrepair")

    os.makedirs(output_dir, exist_ok=True)
    total_bytes = 0

    if not os.path.exists(datasets_dir):
        print(f"Directory not found: {datasets_dir}")
        return

    for root, _, files in os.walk(datasets_dir):
        for filename in files:
            # Skip artifact files if they exist in the source directory
            if any(filename.endswith(suffix) for suffix in SKIP_SUFFIXES):
                continue

            source_dataset_path = os.path.abspath(os.path.join(root, filename))
            local_target_path = os.path.abspath(
                os.path.join(datasets_dir, filename)
            )

            # Copy file to ../datasets/ root if it's located in a subdirectory
            if source_dataset_path != local_target_path:
                shutil.copyfile(source_dataset_path, local_target_path)

            # Standard target generated files
            target_files = [
                f"{local_target_path}.C",
                f"{local_target_path}.R",
            ]

            cmd_compress = f"'{bigrepair_bin}' '{local_target_path}'"
            cmd_decompress = f"'{bigrepair_bin}' -d '{local_target_path}'"

            # Execute sequential pipeline: Repair -> Bigrepair Compress -> Bigrepair Decompress
            full_command = f"{cmd_compress} && {cmd_decompress}"

            try:
                result = subprocess.run(
                    full_command,
                    shell=True,
                    check=True,
                    capture_output=True,
                    text=True,
                    cwd=datasets_dir,
                )

                # Measure sizes of standard encoding outputs
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

                # Write detailed report
                report_path = os.path.join(output_dir, f"{filename}_bigrepair.txt")
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
                # 1. Clean up all generated artifacts matching SKIP_SUFFIXES
                cleanup_generated_files(local_target_path)

                # 2. Remove working copy from root if copied from subfolder
                if (
                    source_dataset_path != local_target_path
                    and os.path.exists(local_target_path)
                ):
                    os.remove(local_target_path)

    print(f"Total Encoding: {total_bytes:,} bytes")


if __name__ == "__main__":
    main()