#!/bin/bash
PLUGIN_PATH="vaporeonpass/VaporeonPass.so"
TEST_DIR="tests"

if [ ! -f "$PLUGIN_PATH" ]; then
    echo "Error: LLVM pass plugin not found at $PLUGIN_PATH"
    exit 1
fi

if [ ! -d "$TEST_DIR" ]; then
    echo "Error: Test directory $TEST_DIR not found"
    exit 1
fi

for c_file in "$TEST_DIR"/*.c; do
    base_name=$(basename -- "$c_file" .c)

    echo "RUNNING TEST $base_name"

    clang -emit-llvm -S "$c_file" -Xclang -disable-O0-optnone -o "$TEST_DIR/$base_name.ll"

    opt -disable-output -load-pass-plugin="$PLUGIN_PATH" -passes="vaporeonpass" "$TEST_DIR/$base_name.ll" > "$TEST_DIR/${base_name}_output.txt"

    echo "Output for $base_name written to ${base_name}_output.txt"
done

echo "All tests completed."
