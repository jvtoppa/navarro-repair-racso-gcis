# GCIS Lite

This project is a new implementation of the GCIS algorithm adapted from Yuta Mori's sais-lite. The main goal is to perform better than original GCIS in both time and memory. However, the compression ratio difers from the original one since this algorithm uses the LMS-substrings' lexnames strictly as proposed in SAIS algorithm, while GCIS doesn't take in consideration the last character of their definition. Therefore, the expected compression ratio for gcis-lite is worse than original GCIS.

## Installation

To install this project, you need to download the repo into your computer and then execute

```BASH
source build.sh
```

After this, you can use the BASH functions declared on `set_project_functions.sh`:

- gcis-config: Build everything
- gcis-main: Configure without tests
- gcis-tests: Configure only tests
- gcis-build: Build
- gcis-test: Run tests
- gcis-clean: Clean cmake
- gcis-rebuild: clean + config + build
- measure: Reports the time and memory usage of a command.

The `build.sh` script will execute `gcis-config` to build the complete project.

Finally, you can use the file `./build/gcis_lite` to compress/decompress a file using one of the following two encoders:

- Elias-Fano
- Simple8b

It is possible to add your own encoder but you will need to implement it with classes following the code's structure.

## Compression

To compress a file called `input.txt` and write the compressed content into another file called `output.txt` you can execute the following command:

```BASH
./build/gcis_lite -c input.txt output.txt <encoder>
```

Where `<encoder>` should be either `-ef` (Elias-Fano) or `-s8b` (Simple8b).

## Decompression

To decompress a file called `compressed.txt` and write the compressed content into another file called `decompressed.txt` you can execute the following command:

```BASH
./build/gcis_lite -d compressed.txt decompressed.txt <encoder>
```

Where `<encoder>` should be either `-ef` (Elias-Fano) or `-s8b` (Simple8b).

## Benchmarking scripts

Benchmarking scripts are provided as follows:

- `download_pizzachilli.sh {-real|-pseudo-real|-logs|-artificial|-all}`: Downloads data from Pizza & Chilli repetitive corpus.
- `test_compression.sh {-real|-artificial|-logs|-pseudo-real|-all} {-ef|-s8b|-all}`: Tests compression on Pizza & Chilli downloaded data.
- `test_decompression.sh {-real|-artificial|-logs|-pseudo-real|-all} {-ef|-s8b|-all}`: Tests decompression on Pizza & Chilli downloaded data.