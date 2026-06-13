# Makefile for host-side emulation build
# Run from the emulate/ directory:
#   make        → build
#   make run    → build and run
#   make clean  → remove build artifacts

CXX      = g++
CXXFLAGS = -std=c++17 -DEMULATE -Wall -Wextra -g \
           -I. \
           -I..

TARGET   = emulate
SRC      = main_emulate.cpp

$(TARGET): $(SRC) hal_mock.hpp systick_emulate.hpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)
	@echo "Build complete → ./$(TARGET)"

run: $(TARGET)
	@echo ""
	./$(TARGET)

clean:
	rm -f $(TARGET) templog_emulated.csv

.PHONY: run clean
