# Vaporeon LLVM Pass

<!-- Add Vaporeon image with color border -->
<p align="center">
  <img src="vaporeon.png" alt="Vaporeon" width="200" style="border-radius: 50%; border: 5px solid #4CAF50;"/>
</p>

<h1 align="center">
  <span style="color: #4CAF50;">Vaporeon LLVM Pass</span>
</h1>

## Overview

The Vaporeon LLVM Pass is designed to add bounds checking and fat pointer propagation to LLVM IR code. It ensures memory safety by tracking pointer bounds and inserting runtime checks to trap out-of-bounds memory accesses.

## Features

- Bounds propagation for pointer variables
- Fat pointer representation for pointer variables
- Runtime bounds checking on memory writes
- Trap block for handling out-of-bounds accesses

## Installation

1. Clone the LLVM source code repository.
2. Place the `VaporeonPass.cpp` file in the `llvm` directory.
3. Add `add_subdirectory(VaporeonPass)` to `llvm/CMakeLists.txt`.
4. Build LLVM following the standard LLVM build instructions.

## Usage

To use the Vaporeon Pass in LLVM, follow these steps:

1. Build LLVM with the Vaporeon Pass included.
2. Use the LLVM opt tool with the `-passes=vaporeonpass` option to run the Vaporeon Pass on LLVM IR code.  
3. Alternatively `sh run.sh` to run all test cases.  
