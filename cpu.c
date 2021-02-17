#include "cpu.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int32_t *cpuCreateMemory(FILE *program, size_t stackCapacity, int32_t **stackBottom)
{
    assert(program != NULL);
    assert(stackBottom != NULL);

    int32_t *memoryPointer = calloc(1024, sizeof(int32_t));
    int32_t *memoryPointerCopy; // Copy used in "free" functions
    if (memoryPointer == NULL) {
        return NULL; // Not enough memory on heap
    }

    size_t allocatedBlocks = 1;
    uint32_t actualIndex = 0;
    uint32_t maxIndex = 1023;
    int32_t input = 0;
    int32_t instruction[4] = { 0 };
    int8_t counter = 3;

    while ((input = fgetc(program)) != EOF) {
        instruction[counter] = input;
        counter--;
        if (counter < 0) {
            if (maxIndex < actualIndex) { // True = Not enough memory for instructions
                allocatedBlocks++;
                memoryPointerCopy = memoryPointer;
                memoryPointer = realloc(memoryPointer, allocatedBlocks * 1024 * sizeof(int32_t));
                if (memoryPointer == NULL) {
                    free(memoryPointerCopy);
                    memoryPointerCopy = NULL;
                    return NULL; // Not enough memory on heap
                }
                maxIndex += 1024;
            }

            for (int8_t i = 0; i < 4; i++) {
                *(memoryPointer + actualIndex) = (*(memoryPointer + actualIndex) << 8) | (instruction[i] & 0xff);
            }
            actualIndex++;
            counter = 3;
        }
    }

    if (counter != 3 || (actualIndex == 0 && stackCapacity == 0)) {
        free(memoryPointer);
        memoryPointer = NULL;
        memoryPointerCopy = NULL;
        return NULL; // Invalid amount of instructions or stack capacity
    }

    size_t memoryLeft = maxIndex - actualIndex + 1;

    if (memoryLeft < stackCapacity) { // True = Not enough memory for stack
        while (memoryLeft < stackCapacity) {
            memoryLeft += 1024;
            allocatedBlocks++;
        }
        memoryPointerCopy = memoryPointer;
        memoryPointer = realloc(memoryPointer, allocatedBlocks * 1024 * sizeof(int32_t));
        if (memoryPointer == NULL) {
            free(memoryPointerCopy);
            memoryPointerCopy = NULL;
            return NULL; // Not enough memory on heap
        }
        maxIndex = allocatedBlocks * 1024 - 1;
    }
    *stackBottom = memoryPointer + maxIndex;
    memset(memoryPointer + actualIndex, 0, (maxIndex - actualIndex + 1) * sizeof(int32_t));
    return memoryPointer;
}

static void resetRegisters(struct cpu *cpu)
{
    cpu->A = 0;
    cpu->B = 0;
    cpu->C = 0;
    cpu->D = 0;
    cpu->status = 0;
    cpu->stackSize = 0;
    cpu->instructionPointer = 0;

#ifdef BONUS_JMP
    cpu->result = 0;
#endif
}

void cpuCreate(struct cpu *cpu, int32_t *memory, int32_t *stackBottom, size_t stackCapacity)
{
    assert(cpu != NULL);
    assert(memory != NULL);
    assert(stackBottom != NULL);

    resetRegisters(cpu);
    cpu->memory = memory;
    cpu->stackBottom = stackBottom;
    cpu->stackLimit = stackBottom - stackCapacity;
}

void cpuDestroy(struct cpu *cpu)
{
    assert(cpu != NULL);

    free(cpu->memory);
    resetRegisters(cpu);
    cpu->memory = NULL;
    cpu->stackBottom = NULL;
    cpu->stackLimit = NULL;
}

void cpuReset(struct cpu *cpu)
{
    assert(cpu != NULL);

    memset(cpu->stackLimit + 1, 0, (cpu->stackBottom - cpu->stackLimit) * sizeof(int32_t));
    resetRegisters(cpu);
}

int cpuStatus(struct cpu *cpu)
{
    assert(cpu != NULL);
    return cpu->status;
}

int32_t cpuPeek(struct cpu *cpu, char reg)
{
    assert(cpu != NULL);

#ifdef BONUS_JMP
    if (reg == 'R') {
        return cpu->result;
    }
#endif

    switch (reg) {
    case 'A':
        return cpu->A;
    case 'B':
        return cpu->B;
    case 'C':
        return cpu->C;
    case 'D':
        return cpu->D;
    case 'S':
        return cpu->stackSize;
    case 'I':
        return cpu->instructionPointer;
    default:
        return 0; // Invalid register
    }
}

int cpuRun(struct cpu *cpu, size_t steps)
{
    assert(cpu != NULL);

    if (cpuStatus(cpu) != cpuOK) {
        return 0;
    }

    uint32_t counter = 0;
    while (steps > counter) {
        cpuStep(cpu);
        counter++;

        if (cpu->status == cpuHalted) {
            break;
        }
        if (cpu->status != cpuOK) {
            return counter * (-1); // Error code
        }
    }
    return counter; // OK
}

static int32_t next32Bits(struct cpu *cpu, int32_t addition)
{
    if (cpu->memory + cpu->instructionPointer + addition > cpu->stackLimit) {
        cpu->status = cpuInvalidAddress;
        return 0;
    }
    return *(cpu->memory + cpu->instructionPointer + addition);
}

static int32_t getRegister(struct cpu *cpu, int32_t index)
{
#ifdef BONUS_JMP
    if (index == 4) {
        return cpu->result;
    }
#endif

    switch (index) {
    case 0:
        return cpu->A;
    case 1:
        return cpu->B;
    case 2:
        return cpu->C;
    case 3:
        return cpu->D;
    }
    return 0;
}

static int operationInstruction(struct cpu *cpu, char operand)
{
    int32_t registerIndex = next32Bits(cpu, 1);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    if (registerIndex < 0 || registerIndex > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

    int32_t registerInValue = getRegister(cpu, registerIndex);
    if (operand == '/' && registerInValue == 0) {
        cpu->status = cpuDivByZero;
        return 0;
    }

    int32_t value = cpuPeek(cpu, 'A');

    switch (operand) {
    case '+':
        value += registerInValue;
        break;
    case '-':
        value -= registerInValue;
        break;
    case '*':
        value *= registerInValue;
        break;
    case '/':
        value /= registerInValue;
        break;
    }
    cpu->A = value;

#ifdef BONUS_JMP
    cpu->result = value;
#endif

    cpu->instructionPointer += 2;
    return 1;
}

static int decIncInstruction(struct cpu *cpu, int value)
{
    int32_t registerIndex = next32Bits(cpu, 1);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    if (registerIndex < 0 || registerIndex > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

    switch (registerIndex) {
    case 0:
        cpu->A += value;
        break;
    case 1:
        cpu->B += value;
        break;
    case 2:
        cpu->C += value;
        break;
    case 3:
        cpu->D += value;
        break;
    }

#ifdef BONUS_JMP
    cpu->result = getRegister(cpu, registerIndex);
#endif

    cpu->instructionPointer += 2;
    return 1;
}

static int loopInstruction(struct cpu *cpu)
{
    if (cpuPeek(cpu, 'C') != 0) {
        int32_t jumpIndex = next32Bits(cpu, 1);
        if (cpuStatus(cpu) == cpuInvalidAddress) {
            return 0;
        }

        cpu->instructionPointer = jumpIndex;
        return 1;
    }
    cpu->instructionPointer += 2;
    return 1;
}

static void setRegister(struct cpu *cpu, int32_t index, int32_t value)
{
    switch (index) {
    case 0:
        cpu->A = value;
        break;
    case 1:
        cpu->B = value;
        break;
    case 2:
        cpu->C = value;
        break;
    case 3:
        cpu->D = value;
        break;
    }
}

static int movInstruction(struct cpu *cpu)
{
    int32_t registerIndex = next32Bits(cpu, 1);
    int32_t numValue = next32Bits(cpu, 2);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    if (registerIndex < 0 || registerIndex > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

    setRegister(cpu, registerIndex, numValue);
    cpu->instructionPointer += 3;
    return 1;
}

static int loadStoreInstruction(struct cpu *cpu, char type)
{
    int32_t registerIndex = next32Bits(cpu, 1);
    int32_t numValue = next32Bits(cpu, 2);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

#ifdef BONUS_JMP
    if (registerIndex == 4 && type == 'l') {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

    int32_t specialIndex = registerIndex;
    if (specialIndex == 4) {
        registerIndex--;
    }
#endif

    if (registerIndex < 0 || registerIndex > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

#ifdef BONUS_JMP
    if (specialIndex == 4) {
        registerIndex++;
    }
#endif

    int32_t shiftValue = numValue + cpuPeek(cpu, 'D');
    if (cpu->stackSize - shiftValue - 1 < 0 || shiftValue < 0) {
        cpu->status = cpuInvalidStackOperation;
        return 0;
    }

    shiftValue = cpu->stackSize - shiftValue - 1;

    if (type == 'l') { // load
        int32_t value = *(cpu->stackBottom - shiftValue);
        setRegister(cpu, registerIndex, value);
    }

    if (type == 's') { // store
        int32_t value = getRegister(cpu, registerIndex);
        *(cpu->stackBottom - shiftValue) = value;
    }

    cpu->instructionPointer += 3;
    return 1;
}

static int inInstruction(struct cpu *cpu)
{
    int32_t registerIndex = next32Bits(cpu, 1);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    if (registerIndex < 0 || registerIndex > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

    int64_t value = 0;
    int scanStatus = scanf("%ld", &value);

    if (scanStatus == EOF) {
        cpu->C = 0;
        setRegister(cpu, registerIndex, -1);
    } else if (scanStatus == 0 || value > INT32_MAX || value < INT32_MIN) { // Invalid input
        cpu->status = cpuIOError;
        return 0;
    } else {
        setRegister(cpu, registerIndex, value);
    }

    cpu->instructionPointer += 2;
    return 1;
}

static int getInstruction(struct cpu *cpu)
{
    int32_t registerIndex = next32Bits(cpu, 1);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    if (registerIndex < 0 || registerIndex > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

    unsigned char value = 0;
    if (scanf("%c", &value) == EOF) {
        cpu->C = 0;
        setRegister(cpu, registerIndex, -1);
    } else {
        setRegister(cpu, registerIndex, value);
    }

    cpu->instructionPointer += 2;
    return 1;
}

static int outPutInstruction(struct cpu *cpu, char type)
{
    int32_t registerIndex = next32Bits(cpu, 1);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

#ifdef BONUS_JMP
    int32_t specialIndex = registerIndex;
    if (specialIndex == 4) {
        registerIndex--;
    }
#endif

    if (registerIndex < 0 || registerIndex > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

#ifdef BONUS_JMP
    if (specialIndex == 4) {
        registerIndex++;
    }
#endif

    int32_t registerValue = getRegister(cpu, registerIndex);

    if (type == 'p') {
        if (registerValue >= 256 || registerValue < 0) {
            cpu->status = cpuIllegalOperand;
            return 0;
        }
        printf("%c", registerValue);
    }

    if (type == 'o') {
        printf("%d", registerValue);
    }
    cpu->instructionPointer += 2;
    return 1;
}

static int swapInstruction(struct cpu *cpu)
{
    int32_t registerIndex1 = next32Bits(cpu, 1);
    int32_t registerIndex2 = next32Bits(cpu, 2);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    if (registerIndex1 < 0 || registerIndex1 > 3 || registerIndex2 < 0 || registerIndex2 > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

    int32_t swap = getRegister(cpu, registerIndex1);
    setRegister(cpu, registerIndex1, getRegister(cpu, registerIndex2));
    setRegister(cpu, registerIndex2, swap);
    cpu->instructionPointer += 3;
    return 1;
}

static int pushInstruction(struct cpu *cpu)
{
    int32_t registerIndex = next32Bits(cpu, 1);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

#ifdef BONUS_JMP
    int32_t specialIndex = registerIndex;
    if (specialIndex == 4) {
        registerIndex--;
    }
#endif

    if (registerIndex < 0 || registerIndex > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

#ifdef BONUS_JMP
    if (specialIndex == 4) {
        registerIndex++;
    }
#endif

    int32_t registerValue = getRegister(cpu, registerIndex);

    if (cpu->stackBottom - cpu->stackSize <= cpu->stackLimit) {
        cpu->status = cpuInvalidStackOperation;
        return 0;
    }

    *(cpu->stackBottom - cpu->stackSize) = registerValue;
    cpu->stackSize++;
    cpu->instructionPointer += 2;
    return 1;
}

static int popInstruction(struct cpu *cpu)
{
    if (cpu->stackSize <= 0) {
        cpu->status = cpuInvalidStackOperation;
        return 0;
    }

    int32_t registerIndex = next32Bits(cpu, 1);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    if (registerIndex < 0 || registerIndex > 3) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

    int32_t value = *(cpu->stackBottom - cpu->stackSize + 1);

    setRegister(cpu, registerIndex, value);
    cpu->stackSize--;
    cpu->instructionPointer += 2;
    return 1;
}

#ifdef BONUS_JMP
static int jumpInstruction(struct cpu *cpu, char type)
{
    int32_t jumpIndex = next32Bits(cpu, 1);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    switch (type) {
    case 'i':
        cpu->instructionPointer = jumpIndex;
        return 1;
    case 'e':
        if (cpuPeek(cpu, 'R') == 0) {
            cpu->instructionPointer = jumpIndex;
            return 1;
        }
        break;
    case 'n':
        if (cpuPeek(cpu, 'R') != 0) {
            cpu->instructionPointer = jumpIndex;
            return 1;
        }
        break;
    case 'b':
        if (cpuPeek(cpu, 'R') > 0) {
            cpu->instructionPointer = jumpIndex;
            return 1;
        }
        break;
    }

    cpu->instructionPointer += 2;
    return 1;
}

static int cmpInstruction(struct cpu *cpu)
{
    int32_t registerIndexFirst = next32Bits(cpu, 1);
    int32_t registerIndexSecond = next32Bits(cpu, 2);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    if (registerIndexFirst < 0 || registerIndexFirst > 4 || registerIndexSecond < 0 || registerIndexSecond > 4) {
        cpu->status = cpuIllegalOperand;
        return 0;
    }

    cpu->result = getRegister(cpu, registerIndexFirst) - getRegister(cpu, registerIndexSecond);
    cpu->instructionPointer += 3;
    return 1;
}
#endif

#ifdef BONUS_CALL
static int callInstruction(struct cpu *cpu)
{
    int32_t jumpIndex = next32Bits(cpu, 1);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

    if (cpu->stackBottom - cpu->stackSize <= cpu->stackLimit) {
        cpu->status = cpuInvalidStackOperation;
        return 0;
    }

    *(cpu->stackBottom - cpu->stackSize) = cpu->instructionPointer + 2;
    cpu->instructionPointer = jumpIndex;
    cpu->stackSize++;
    return 1;
}

static int retInstruction(struct cpu *cpu)
{
    if (cpu->stackSize <= 0) {
        cpu->status = cpuInvalidStackOperation;
        return 0;
    }

    cpu->instructionPointer = *(cpu->stackBottom - cpu->stackSize + 1);
    cpu->stackSize--;
    return 1;
}
#endif

enum instructionKeyWords
{
    nope,
    halt,
    add,
    sub,
    mul,
    divI,
    inc,
    dec,
    loop,
    mov,
    load,
    store,
    in,
    get,
    out,
    put,
    swap,
    push,
    pop,
    cmp,
    jmp,
    jz,
    jnz,
    jgt,
    call,
    ret
};

int cpuStep(struct cpu *cpu)
{
    assert(cpu != NULL);

    if (cpuStatus(cpu) != cpuOK) {
        return 0;
    }

    if (cpu->memory + cpu->instructionPointer < cpu->memory ||
            cpu->memory + cpu->instructionPointer > cpu->stackLimit) {
        cpu->status = cpuInvalidAddress;
        return 0;
    }

    int32_t instruction = next32Bits(cpu, 0);
    if (cpuStatus(cpu) == cpuInvalidAddress) {
        return 0;
    }

#ifdef BONUS_JMP
    switch (instruction) {
    case cmp:
        return cmpInstruction(cpu);
    case jmp:
        return jumpInstruction(cpu, 'i'); // Ignore R
    case jz:
        return jumpInstruction(cpu, 'e'); // Equal zero
    case jnz:
        return jumpInstruction(cpu, 'n'); // Not zero
    case jgt:
        return jumpInstruction(cpu, 'b'); // Bigger then zero
    }
#endif

#ifdef BONUS_CALL
    if (instruction == call) {
        return callInstruction(cpu);
    } else if (instruction == ret) {
        return retInstruction(cpu);
    }
#endif

    switch (instruction) {
    case nope:
        cpu->instructionPointer++;
        return 1;
    case halt:
        cpu->status = cpuHalted;
        cpu->instructionPointer++;
        return 0;
    case add:
        return operationInstruction(cpu, '+');
    case sub:
        return operationInstruction(cpu, '-');
    case mul:
        return operationInstruction(cpu, '*');
    case divI:
        return operationInstruction(cpu, '/');
    case inc:
        return decIncInstruction(cpu, 1);
    case dec:
        return decIncInstruction(cpu, -1);
    case loop:
        return loopInstruction(cpu);
    case mov:
        return movInstruction(cpu);
    case load:
        return loadStoreInstruction(cpu, 'l');
    case store:
        return loadStoreInstruction(cpu, 's');
    case in:
        return inInstruction(cpu);
    case get:
        return getInstruction(cpu);
    case out:
        return outPutInstruction(cpu, 'o');
    case put:
        return outPutInstruction(cpu, 'p');
    case swap:
        return swapInstruction(cpu);
    case push:
        return pushInstruction(cpu);
    case pop:
        return popInstruction(cpu);
    default:
        cpu->status = cpuIllegalInstruction;
        return 0;
    }
}