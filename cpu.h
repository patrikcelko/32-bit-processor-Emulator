#ifndef CPU_H
#define CPU_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum cpuStatus
{
    cpuOK,
    cpuHalted,
    cpuIllegalInstruction,
    cpuIllegalOperand,
    cpuInvalidAddress,
    cpuInvalidStackOperation,
    cpuDivByZero,
    cpuIOError
};

struct cpu
{
    int32_t A;                  // Register A
    int32_t B;                  // Register B
    int32_t C;                  // Register C
    int32_t D;                  // Register D
    enum cpuStatus status;      // CPU status code (reg.)
    int32_t stackSize;          // Actual amount (reg.)
    int32_t instructionPointer; // Index of next instruction (reg.)
    int32_t *memory;            // Memory start pointer
    int32_t *stackBottom;       // Bottom stack limit
    int32_t *stackLimit;        // Upper stack limit
#ifdef BONUS_JMP
    int32_t result; // Result of operations (reg.)
#endif
};

int32_t *cpuCreateMemory(FILE *program, size_t stackCapacity, int32_t **stackBottom);

void cpuCreate(struct cpu *cpu, int32_t *memory, int32_t *stackBottom, size_t stackCapacity);

void cpuDestroy(struct cpu *cpu);

void cpuReset(struct cpu *cpu);

int cpuStatus(struct cpu *cpu);

int32_t cpuPeek(struct cpu *cpu, char reg);

int cpuStep(struct cpu *cpu);

int cpuRun(struct cpu *cpu, size_t steps);

#endif
