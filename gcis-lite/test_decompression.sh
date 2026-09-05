#!/usr/bin/env bash

set -e

# ==============================
# Validación básica
# ==============================
if [ $# -lt 1 ]; then
    echo "Uso:"
    echo "  $0 {-real|-artificial|-logs|-pseudo-real|-all} {-ef|-s8b|-all}"
    echo "  $0 -clean"
    exit 1
fi

GROUP_MODE="$1"
MODE="$2"

INPUT_DIR="outputs"
OUTPUT_DIR="decompressed_inputs"
LOG_DIR="logs"

# ==============================
# Opción CLEAN
# ==============================
if [ "$GROUP_MODE" = "-clean" ]; then
    echo "Limpiando carpeta '$OUTPUT_DIR'..."
    rm -rf "$OUTPUT_DIR"
    echo "Limpieza completada."
    return 0
fi

# ==============================
# Validar que haya 2 argumentos
# ==============================
if [ $# -ne 2 ]; then
    echo "Uso: $0 {-real|-artificial|-logs|-pseudo-real|-all} {-ef|-s8b|-all}"
    exit 1
fi

# ==============================
# Selección de grupos
# ==============================
unset GROUPS
declare -a GROUPS

case "$GROUP_MODE" in
    -real)
        GROUPS=("real")
        ;;
    -artificial)
        GROUPS=("artificial")
        ;;
    -logs)
        GROUPS=("logs")
        ;;
    -pseudo-real)
        GROUPS=("pseudo-real")
        ;;
    -all)
        GROUPS=("real" "artificial" "logs" "pseudo-real")
        ;;
    *)
        echo "Grupo inválido."
        exit 1
        ;;
esac

# ==============================
# Selección de encoders
# ==============================
case "$MODE" in
    -ef)
        ENCODERS=("-ef")
        ;;
    -s8b)
        ENCODERS=("-s8b")
        ;;
    -all)
        ENCODERS=("-ef" "-s8b")
        ;;
    *)
        echo "Encoder inválido."
        exit 1
        ;;
esac

# ==============================
# Verificar measure
# ==============================
if ! command -v measure &> /dev/null; then
    if [ -f "./set_project_functions.sh" ]; then
        source ./set_project_functions.sh
    else
        echo "No se encontró set_project_functions.sh"
        exit 1
    fi
fi

if ! command -v measure &> /dev/null; then
    echo "Error: 'measure' no está definido."
    exit 1
fi

# ==============================
# Preparar entorno
# ==============================
if [ ! -d "$INPUT_DIR" ]; then
    echo "La carpeta '$INPUT_DIR' no existe."
    exit 1
fi

mkdir -p "$OUTPUT_DIR"
mkdir -p "$LOG_DIR"

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="${LOG_DIR}/benchmark_decompression_${GROUP_MODE}_${MODE}_${TIMESTAMP}.log"

echo "=========================================" | tee -a "$LOG_FILE"
echo "Grupos: $GROUP_MODE" | tee -a "$LOG_FILE"
echo "Encoders: $MODE" | tee -a "$LOG_FILE"
echo "Fecha inicio: $(date)" | tee -a "$LOG_FILE"
echo "=========================================" | tee -a "$LOG_FILE"

# ==============================
# Ejecutar benchmarks
# ==============================

for GROUP in "${GROUPS[@]}"; do

    GROUP_PATH="${INPUT_DIR}/${GROUP}"

    if [ ! -d "$GROUP_PATH" ]; then
        echo "Grupo no encontrado en outputs: $GROUP_PATH"
        continue
    fi

    echo "" | tee -a "$LOG_FILE"
    echo "=========== Grupo: $GROUP ===========" | tee -a "$LOG_FILE"

    find "$GROUP_PATH" -type f | while read -r compressed_file; do

        base_name=$(basename "$compressed_file")

        for ENC in "${ENCODERS[@]}"; do

            # Solo procesar archivos que correspondan al encoder
            if [[ "$compressed_file" != *"$ENC.out" ]]; then
                continue
            fi

            output_file="${OUTPUT_DIR}/${GROUP}/${base_name%.out}_decompressed.txt"

            mkdir -p "${OUTPUT_DIR}/${GROUP}"

            echo "" | tee -a "$LOG_FILE"
            echo "Input (compressed): $compressed_file" | tee -a "$LOG_FILE"
            echo "Encoder: $ENC" | tee -a "$LOG_FILE"
            echo "Output (decompressed): $output_file" | tee -a "$LOG_FILE"
            echo "-----------------------------------------" | tee -a "$LOG_FILE"

            echo "gcis_lite performance:" >> "$LOG_FILE"

            measure ./build/gcis_lite -d "$compressed_file" "$output_file" "$ENC" >> "$LOG_FILE" 2>&1

            if [ -f ../GCIS/build/src/gcis ]; then

                echo "" >> "$LOG_FILE"
                echo "gcis performance:" >> "$LOG_FILE"

                measure ../GCIS/build/src/gcis -d "$compressed_file" "$output_file" "$ENC" >> "$LOG_FILE" 2>&1
            else
                echo "Not comparing gcis_lite and GCIS, only generating gcis_lite performance" >> "$LOG_FILE"
            fi

            echo "-----------------------------------------" | tee -a "$LOG_FILE"

        done

    done

done

echo "" | tee -a "$LOG_FILE"
echo "Benchmark decompresión finalizado: $(date)" | tee -a "$LOG_FILE"
echo "Log guardado en: $LOG_FILE"