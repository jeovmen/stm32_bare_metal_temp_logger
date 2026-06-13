# =============================================================================
# temp_logger — QEMU & utility targets
#
# Usage:
#   make qemu        — run elf in QEMU (prints FSM output to terminal)
#   make qemu-debug  — same but pauses for GDB on port 1234
#   make clean-emu   — remove emulation build artifacts
# =============================================================================

# Path to the CubeIDE Debug build output
ELF = Debug/temp_logger.elf

# QEMU binary
QEMU = qemu-system-arm

# STM32F4 Cortex-M4F machine — closest available to F446RE in QEMU
MACHINE = netduinoplus2

# -----------------------------------------------------------------------------
# qemu — boot the elf, USART2 mapped to stdio, no display window
# -----------------------------------------------------------------------------
.PHONY: qemu
qemu: $(ELF)
	@echo "Starting QEMU — press Ctrl+A then X to quit"
	@echo "-------------------------------------------"
	$(QEMU) \
		-M $(MACHINE) \
		-kernel $(ELF) \
		-serial null \
		-serial mon:stdio \
		-nographic

# -----------------------------------------------------------------------------
# qemu-debug — same as above but halts CPU and waits for GDB on port 1234
# In a second terminal run:
#   arm-none-eabi-gdb Debug/temp_logger.elf
#   (gdb) target remote :1234
#   (gdb) continue
# -----------------------------------------------------------------------------
.PHONY: qemu-debug
qemu-debug: $(ELF)
	@echo "QEMU paused — connect GDB on port 1234"
	$(QEMU) \
		-M $(MACHINE) \
		-kernel $(ELF) \
		-serial null \
		-serial mon:stdio \
		-nographic \
		-s -S

# -----------------------------------------------------------------------------
# check — verify QEMU and the elf both exist before trying to run
# -----------------------------------------------------------------------------
.PHONY: check
check:
	@which $(QEMU) > /dev/null 2>&1 || \
		(echo "ERROR: qemu-system-arm not found. Run: sudo apt install qemu-system-arm" && exit 1)
	@test -f $(ELF) || \
		(echo "ERROR: $(ELF) not found. Build the project in CubeIDE first." && exit 1)
	@echo "OK: QEMU found at $$(which $(QEMU))"
	@echo "OK: ELF found at $(ELF)"

# -----------------------------------------------------------------------------
# Guard — if elf is missing, print a helpful message instead of a cryptic error
# -----------------------------------------------------------------------------
$(ELF):
	@echo "ERROR: $(ELF) not found."
	@echo "Build the project in STM32CubeIDE first (Project -> Build All)."
	@exit 1
