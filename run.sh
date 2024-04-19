#!/bin/bash
clang -emit-llvm -S simple.c -Xclang -disable-O0-optnone -o simple.ll
opt -disable-output -load-pass-plugin=vaporeonpass/vaporeonpass/VaporeonPass.so -passes="vaporeonpass" simple.ll