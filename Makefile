CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
BONUS_FLAGS = -DBONUS_JMP -DBONUS_CALL
TARGET = cpu
COMPILER_TARGET = compiler
CPU_SOURCES = main.c cpu.c compiler.c
COMPILER_SOURCES = compiler.c
HEADERS = cpu.h
CPU_OBJECTS = $(CPU_SOURCES:.c=.o)

# Build CPU emulator with all features
all: $(TARGET)

# Build CPU emulator with bonus features
$(TARGET): $(CPU_SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(BONUS_FLAGS) -o $(TARGET) $(CPU_SOURCES)

# Build standalone compiler
$(COMPILER_TARGET): $(COMPILER_SOURCES)
	$(CC) $(CFLAGS) -o $(COMPILER_TARGET) $(COMPILER_SOURCES)

# Build CPU without bonus features
cpu-basic: $(CPU_SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(CPU_SOURCES)

# Build CPU with only jump support
cpu-jump: $(CPU_SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DBONUS_JMP -o $(TARGET) $(CPU_SOURCES)

# Build CPU with only call/return support
cpu-call: $(CPU_SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -DBONUS_CALL -o $(TARGET) $(CPU_SOURCES)

# Build with debug symbols
debug: CFLAGS += -g -O0
debug: $(TARGET)

# Build optimized version
release: CFLAGS += -O3
release: $(TARGET)

# Run tests
test: $(TARGET)
	@echo "Running test programs..."
	@for test in Tests/*.bin; do \
		echo "Testing $$test..."; \
		./$(TARGET) run "$$test" || true; \
	done

# Clean build artifacts
clean:
	rm -f $(TARGET) $(COMPILER_TARGET) *.o *~ core

# Clean and rebuild
rebuild: clean all

# Install (optional - copies to /usr/local/bin)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Show help
help:
	@echo "32-bit Processor Emulator - Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build CPU emulator with all features (default)"
	@echo "  make all          - Same as above"
	@echo "  make compiler     - Build standalone assembly compiler"
	@echo "  make cpu-basic    - Build CPU without bonus features"
	@echo "  make cpu-jump     - Build CPU with jump support only"
	@echo "  make cpu-call     - Build CPU with call/return support only"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make release      - Build optimized version"
	@echo "  make test         - Run test programs"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make install      - Install to /usr/local/bin (requires sudo)"
	@echo "  make uninstall    - Remove from /usr/local/bin (requires sudo)"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  ./cpu run program.bin"
	@echo "  ./cpu run 512 program.bin"
	@echo "  ./cpu trace program.bin"

.PHONY: all clean rebuild test debug release install uninstall help cpu-basic cpu-jump cpu-call compiler
