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

if [ -z "$1" ]; then
    for c_file in "$TEST_DIR"/*.c; do
        base_name=$(basename -- "$c_file" .c)

        echo "RUNNING TEST $base_name"

        clang -emit-llvm -S "$c_file" -Xclang -disable-O0-optnone -o "$TEST_DIR/$base_name.ll"

        opt -load-pass-plugin="$PLUGIN_PATH" -passes="vaporeonpass" "$TEST_DIR/$base_name.ll" -o "$TEST_DIR/$base_name.vaporeon.ll" > "$TEST_DIR/${base_name}_output.txt"
        clang "$TEST_DIR/$base_name.ll" -o $base_name
        clang "$TEST_DIR/$base_name.vaporeon.ll" -o $base_name.vaporeon

        echo "Output for $base_name written to ${base_name}_output.txt"
    done
else
    for c_file in "$@"; do
        base_name=$(basename -- "$c_file" .c)

        echo "RUNNING TEST $base_name"

        clang -emit-llvm -S "$c_file" -Xclang -disable-O0-optnone -o "$TEST_DIR/$base_name.ll"

        opt -S -load-pass-plugin="$PLUGIN_PATH" -passes="vaporeonpass" "$TEST_DIR/$base_name.ll" -o "$TEST_DIR/$base_name.vaporeon.ll" > "$TEST_DIR/${base_name}_output.txt"
        clang "$TEST_DIR/$base_name.ll" -o $base_name
        clang "$TEST_DIR/$base_name.vaporeon.ll" -o $base_name.vaporeon

        echo "Output for $base_name written to ${base_name}_output.txt"
    done
fi

echo "All tests completed."
