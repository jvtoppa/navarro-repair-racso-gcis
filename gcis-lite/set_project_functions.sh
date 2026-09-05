# ========================================
# GCIS CMake Workflow (Robust Version)
# ========================================
# Default build directory
GCIS_BUILD_DIR="build"

# Configure (default: build everything)
gcis-config() {
    cmake -S . -B "$GCIS_BUILD_DIR" -DCMAKE_BUILD_TYPE=Release "$@" &&
    cmake --build "$GCIS_BUILD_DIR" -j
}

# Configure without tests
gcis-main() {
    cmake -S . -B "$GCIS_BUILD_DIR" \
        -DBUILD_TESTS=OFF \
        -DBUILD_MAIN=ON \
        "$@" &&
    cmake --build "$GCIS_BUILD_DIR" -j
}

# Configure only tests
gcis-tests() {
    cmake -S . -B "$GCIS_BUILD_DIR" \
        -DBUILD_MAIN=OFF \
        -DBUILD_TESTS=ON \
        "$@" &&
    cmake --build "$GCIS_BUILD_DIR" -j
}

# Build
gcis-build() {
    cmake --build "$GCIS_BUILD_DIR" -j
}

# Run tests
gcis-test() {
    ctest --test-dir "$GCIS_BUILD_DIR"
}

# Clean everything
gcis-clean() {
    rm -rf "$GCIS_BUILD_DIR" CMakeFiles CMakeCache.txt
}

# Full rebuild (clean + configure + build)
gcis-rebuild() {
    gcis-clean
    gcis-config "$@"
}

measure() {

  TIME_CMD="/usr/bin/time"

  if ! "$TIME_CMD" --version &> /dev/null; then
    echo "Error: Se requiere GNU time."
    return 1
  fi

  TMP_OUTPUT=$(mktemp)

  # -o escribe SOLO las métricas en el archivo
  "$TIME_CMD" -f "%E|%U|%S|%K|%M" -o "$TMP_OUTPUT" "$@"

  IFS="|" read -r REAL USER SYS AVG_RAW PEAK_RAW < "$TMP_OUTPUT"
  rm -f "$TMP_OUTPUT"

  # GNU time → %K y %M están en KB
  AVG_BYTES=$((AVG_RAW * 1024))
  PEAK_BYTES=$((PEAK_RAW * 1024))

  echo ""
  echo "Real Time: $REAL"
  echo "CPU Time: $USER user $SYS sys"
  echo "Average Memory (Bytes): $AVG_BYTES"
  echo "Peak Memory (Bytes): $PEAK_BYTES"
}