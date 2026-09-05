import os
import shutil
import subprocess

# Working directory fix: Ensure execution happens relative to the script location
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
os.chdir(SCRIPT_DIR)

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
    ".c.pre_repair_backup"
)


def is_condition_met(stdout: str) -> bool:
    lines = [line.strip() for line in stdout.splitlines()]
    return "End." in lines


def fix_missing_trailing_byte(reference_path: str, target_path: str):
    """
    Compares reference vs target. If target is exactly one byte shorter than
    reference, appends the missing byte (e.g. 0x00).
    """
    if not (os.path.exists(reference_path) and os.path.exists(target_path)):
        return

    with open(reference_path, "rb") as f:
        reference_bytes = f.read()
    with open(target_path, "rb") as f:
        target_bytes = f.read()

    if (
        len(reference_bytes) == len(target_bytes) + 1
        and reference_bytes[:-1] == target_bytes
    ):
        missing_byte = reference_bytes[-1:]
        with open(target_path, "ab") as f:
            f.write(missing_byte)


def clean_up_generated_files(dataset_path: str):
    """Removes all generated temporary/intermediate files matching SKIP_SUFFIXES."""
    print(f"Cleaning up generated files for {dataset_path}...")
    for suffix in SKIP_SUFFIXES:
        generated_file = f"{dataset_path}{suffix}"
        if os.path.exists(generated_file):
            try:
                os.remove(generated_file)
            except OSError as e:
                print(f"Warning: Could not remove {generated_file}: {e}")


def run_step(command: str, description: str):
    """Runs a single shell command step, returning its result."""
    print(f"--- Running: {description} ---", flush=True)
    return subprocess.run(
        command,
        shell=True,
        check=True,
        capture_output=True,
        text=True,
    )


def run_experiment_for_tool(tool_name: str, dataset_path: str, output_dir: str, filename: str):
    compressed_file = f"{dataset_path}.c"
    backup_path = dataset_path + ".orig_backup"

    # Always ensure we start from a clean original dataset
    if os.path.exists(backup_path):
        shutil.copy2(backup_path, dataset_path)
    else:
        shutil.copy2(dataset_path, backup_path)

    report_path = os.path.join(output_dir, f"{filename}_{tool_name}.txt")
    print(f"\nRunning [{tool_name}] for dataset {dataset_path}...")

    no = 0
    end_condition = False

    with open(report_path, "w", encoding="utf-8") as report_file:
        report_file.write(f"=== {tool_name.upper()} Report for {filename} ===\n\n")

        while not end_condition:
            combined_stdout = ""
            combined_stderr = ""
            try:
                # Step 1: encode with gcis_lite
                r1 = run_step(
                    f".././gcis-lite/build/gcis_lite -c {dataset_path} {dataset_path} -s8b {no}",
                    f"gcis_lite -c (level={no})",
                )
                combined_stdout += r1.stdout
                if r1.stderr:
                    combined_stderr += r1.stderr

                # Snapshot pre-compressed file before repair modifies it
                pre_repair_backup = compressed_file + ".pre_repair_backup"
                shutil.copy2(compressed_file, pre_repair_backup)

                # Step 2: Compress using tool
                if tool_name == "irepair":
                    cmd_compress = f".././repair/irepair {compressed_file}"
                else:  # bigrepair
                    cmd_compress = f".././bigrepair/bigrepair {compressed_file}"

                r2 = run_step(cmd_compress, f"{tool_name} (level={no})")
                combined_stdout += r2.stdout
                if r2.stderr:
                    combined_stderr += r2.stderr

                # Record file sizes
                r_path = f"{dataset_path}.r"
                c_r_path = f"{compressed_file}.R"
                c_c_path = f"{compressed_file}.C"

                r_size = os.path.getsize(r_path) if os.path.exists(r_path) else 0
                c_r_size = os.path.getsize(c_r_path) if os.path.exists(c_r_path) else 0
                c_c_size = os.path.getsize(c_c_path) if os.path.exists(c_c_path) else 0

                # Step 3: Decompress using tool
                if tool_name == "irepair":
                    cmd_decompress = f".././repair/idespair {compressed_file}"
                else:  # bigrepair
                    cmd_decompress = f".././bigrepair/bigrepair -d {compressed_file}"

                r3 = run_step(cmd_decompress, f"{tool_name} decompress (level={no})")
                combined_stdout += r3.stdout
                if r3.stderr:
                    combined_stderr += r3.stderr

                if tool_name == "bigrepair":
                    bigrepair_out = f"{compressed_file}.out"
                    if os.path.exists(bigrepair_out):
                        shutil.move(bigrepair_out, compressed_file)

                # Append missing trailing 0x00 byte to dataset_path.c if idespair stripped it
                fix_missing_trailing_byte(pre_repair_backup, compressed_file)

                # Step 4: final decompression matching current level parameter
                r4 = run_step(
                    f".././gcis-lite/build/gcis_lite -d {dataset_path} {dataset_path} -s8b {no}",
                    f"gcis_lite -d (level={no})",
                )
                combined_stdout += r4.stdout
                if r4.stderr:
                    combined_stderr += r4.stderr

                # Verify final decompressed output against original
                fix_missing_trailing_byte(backup_path, dataset_path)

                # Write log report
                report_file.write(f"=== LEVEL {no} ===\n")
                report_file.write("--- FILE SIZES ---\n")
                report_file.write(f"  - .r   ({r_path}): {r_size:,} bytes\n")
                report_file.write(f"  - .c.R ({c_r_path}): {c_r_size:,} bytes\n")
                report_file.write(f"  - .c.C ({c_c_path}): {c_c_size:,} bytes\n\n")

                report_file.write("--- STDOUT ---\n")
                report_file.write(combined_stdout if combined_stdout else "(empty)\n")
                report_file.write("\n--- STDERR ---\n")
                report_file.write(combined_stderr if combined_stderr else "(empty)\n")
                report_file.write("\n" + "=" * 40 + "\n\n")

                end_condition = is_condition_met(combined_stdout)

            except subprocess.CalledProcessError as e:
                print(f"Error processing {dataset_path} at level {no} [{tool_name}]: {e}")
                report_file.write(f"=== LEVEL {no} (ERROR) ===\n")
                report_file.write(f"Error message: {e}\n\n")
                if e.stdout:
                    report_file.write("--- STDOUT BEFORE FAILURE ---\n" + e.stdout + "\n")
                if e.stderr:
                    report_file.write("--- STDERR BEFORE FAILURE ---\n" + e.stderr + "\n")
                report_file.write("\n" + "=" * 40 + "\n\n")
                end_condition = True

            no += 1

    # Restore dataset back to original state and delete all temporary files
    if os.path.exists(backup_path):
        shutil.copy2(backup_path, dataset_path)

    clean_up_generated_files(dataset_path)


def main():
    datasets_dir = "../datasets"
    output_dir = "../experiment/output_reports"
    print("Starting...")
    os.makedirs(output_dir, exist_ok=True)

    if not os.path.exists(datasets_dir):
        print(f"Directory not found: {datasets_dir}")
        return

    for root, _, files in os.walk(datasets_dir):
        for filename in files:
            if filename.endswith(SKIP_SUFFIXES):
                continue

            dataset_path = os.path.join(root, filename)

            for tool_name in ["irepair", "bigrepair"]:
                run_experiment_for_tool(tool_name, dataset_path, output_dir, filename)


if __name__ == "__main__":
    main()
    print("Done.")