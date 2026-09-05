#!/usr/bin/env bash

# Detectar si está siendo sourced
_is_sourced() {
  [ "${BASH_SOURCE[0]}" != "$0" ]
}

BASE_URL="https://pizzachili.dcc.uchile.cl/repcorpus"
DEST_ROOT="inputs"

if [ $# -ne 1 ]; then
    echo "Uso: download_pizzachilli.sh {-real|-pseudo-real|-logs|-artificial|-all}"
    _is_sourced && return 1 || exit 1
fi

MODE="$1"

unset GROUPS
declare -a GROUPS

case "$MODE" in
    -real) GROUPS=("real") ;;
    -pseudo-real) GROUPS=("pseudo-real") ;;
    -logs) GROUPS=("logs") ;;
    -artificial) GROUPS=("artificial") ;;
    -all) GROUPS=("real" "pseudo-real" "logs" "artificial") ;;
    *)
        echo "Opción inválida."
        _is_sourced && return 1 || exit 1
        ;;
esac

# Verificar 7z
if ! command -v 7z &> /dev/null; then
    echo "7z no encontrado. Instalando p7zip-full..."
    if command -v apt &> /dev/null; then
        sudo apt update
        sudo apt install -y p7zip-full
    else
        echo "No se pudo instalar automáticamente."
        _is_sourced && return 1 || exit 1
    fi
fi

mkdir -p "$DEST_ROOT"

for GROUP in "${GROUPS[@]}"; do

    echo "======================================="
    echo "Descargando grupo: $GROUP"
    echo "======================================="

    URL="${BASE_URL}/${GROUP}/"
    DEST="${DEST_ROOT}/${GROUP}"

    mkdir -p "$DEST"
    cd "$DEST" || { echo "Error al entrar a $DEST"; _is_sourced && return 1 || exit 1; }

    FILES=$(curl -s "$URL" | grep -oP '(?<=href=")[^"]+\.7z')

    echo "Descargando y extrayendo archivos..."

    for file in $FILES; do
        echo "Procesando $file..."

        wget -q "${URL}${file}"
        7z x "$file" -o"$DEST" -y > /dev/null
        rm "$file"
    done

    cd - > /dev/null
done

echo "Descarga finalizada."

# Si fue sourced, devolver control limpio
_is_sourced && return 0 || exit 0