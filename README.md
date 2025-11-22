# 32-bit Processor Emulator

A complete implementation of a 32-bit processor emulator with assembler, virtual CPU, and instruction set architecture. Also includes a custom assembly language compiler and a fully functional CPU simulator that executes binary programs.

## CPU Specifications

- **Architecture**: 32-bit register-based processor
- **Registers**: 4 general-purpose registers (A, B, C, D) + optional result register (R)
- **Memory**: Dynamic allocation with separate instruction and stack memory
- **Stack**: Grows downward from top of memory
- **Instruction Pointer**: Tracks next instruction to execute
- **Status Codes**: 8 different CPU states for error handling

### Status Codes

| Status | Description |
|--------|-------------|
| `cpuOK` | Normal operation |
| `cpuHalted` | Program terminated normally |
| `cpuIllegalInstruction` | Unknown opcode encountered |
| `cpuIllegalOperand` | Invalid register or operand |
| `cpuInvalidAddress` | Memory access violation |
| `cpuInvalidStackOperation` | Stack overflow/underflow |
| `cpuDivByZero` | Division by zero attempted |
| `cpuIOError` | Input/output error |

## Instruction Set

### Arithmetic Operations

| Opcode | Mnemonic | Arguments | Description |
|--------|----------|-----------|-------------|
| 0x2 | `add` | reg | A = A + reg |
| 0x3 | `sub` | reg | A = A - reg |
| 0x4 | `mul` | reg | A = A * reg |
| 0x5 | `div` | reg | A = A / reg |
| 0x6 | `inc` | reg | reg = reg + 1 |
| 0x7 | `dec` | reg | reg = reg - 1 |

### Data Movement

| Opcode | Mnemonic | Arguments | Description |
|--------|----------|-----------|-------------|
| 0x9 | `movr` | reg, num | reg = num |
| 0xa | `load` | reg, num | reg = stack[D + num] |
| 0xb | `store` | reg, num | stack[D + num] = reg |
| 0x10 | `swap` | reg1, reg2 | Swap values of reg1 and reg2 |

### Stack Operations

| Opcode | Mnemonic | Arguments | Description |
|--------|----------|-----------|-------------|
| 0x11 | `push` | reg | Push register value onto stack |
| 0x12 | `pop` | reg | Pop value from stack into register |

### I/O Operations

| Opcode | Mnemonic | Arguments | Description |
|--------|----------|-----------|-------------|
| 0xc | `in` | reg | Read integer from stdin |
| 0xd | `get` | reg | Read single character from stdin |
| 0xe | `out` | reg | Write integer to stdout |
| 0xf | `put` | reg | Write character to stdout |

### Control Flow

| Opcode | Mnemonic | Arguments | Description |
|--------|----------|-----------|-------------|
| 0x0 | `nop` | - | No operation |
| 0x1 | `halt` | - | Stop execution |
| 0x8 | `loop` | label | Jump to label if C ≠ 0 |
| 0x13 | `cmp` | reg1, reg2 | R = reg1 - reg2 |
| 0x14 | `jmp` | label | Unconditional jump |
| 0x15 | `jz` | label | Jump if R = 0 |
| 0x16 | `jnz` | label | Jump if R ≠ 0 |
| 0x17 | `jgt` | label | Jump if R > 0 |
| 0x18 | `call` | label | Call subroutine |
| 0x19 | `ret` | - | Return from subroutine |

## Building

```bash
# Build with all features (default)
make

# Build standalone compiler
make compiler

# Build without bonus features
make cpu-basic

# Build with debug symbols
make debug

# Build optimized version
make release

# Clean build artifacts
make clean

# Show all available targets
make help
```

### Running Programs

```bash
# Run a binary program with default stack (256)
./cpu run program.bin

# Run with custom stack capacity
./cpu run 512 program.bin

# Trace execution step-by-step
./cpu trace program.bin
```

## CPU Registers

| Register | Symbol | Purpose |
|----------|--------|---------|
| A | A/0 | General purpose, accumulator for arithmetic |
| B | B/1 | General purpose |
| C | C/2 | General purpose, loop counter |
| D | D/3 | General purpose, base pointer for load/store |
| R | R/4/result | Result register for comparisons (BONUS_JMP) |
| SP | - | Stack pointer (internal) |
| IP | - | Instruction pointer (internal) |

## Memory Model

```text
┌─────────────────┐ ← Memory start (address 0)
│   Instructions  │
│                 │
├─────────────────┤
│   Free Space    │
│                 │
├─────────────────┤ ← Stack limit
│                 │
│     Stack ↓     │
│                 │
└─────────────────┘ ← Stack bottom (highest address)
```

- **Instructions**: Loaded from binary file at start of memory
- **Stack**: Allocated at end of memory, grows downward
- **Memory allocation**: Dynamic, grows in 1024-word blocks as needed

## License

This project is open source. Feel free to use and modify as needed.
