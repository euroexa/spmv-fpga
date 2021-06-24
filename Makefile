#+-------------------------------------------------------------------------------
# The following parameters are assigned with default values. These parameters can
# be overridden through the make command line
#+-------------------------------------------------------------------------------
# FPGA Board Platform (Default ~ zcu102)
PLATFORM := zcu102

# Run Target:
#   hw  - Compile for hardware
#   emu - Compile for emulation (Default)
TARGET := emu

# Number of Compute Units      : 1, 2, 4, 8, 10, 12
CU := 1
# Vectorization Factor applied : 1, 2, 4, 8
VF := 1
# Double or floating point arithemtic : 1 or 0
DOUBLE := 1

# Current Directory
pwd := $(CURDIR)

# Target OS: linux (Default), standalone
TARGET_OS := linux

# Emulation Mode:
#     debug     - Include debug data
#     optimized - Exclude debug data (Default)
EMU_MODE := optimized

# Additional sds++ flags - this should be reserved for sds++ flags defined at run-time. Other sds++ options should be defined in the makefile data section below section below
ADDL_FLAGS := -Wno-unused-label

# Set to 1 to enable sds++ verbose output
VERBOSE := 1

# Build Executable
EXECUTABLE := run.elf 
# Build Directory

ifeq ($(DOUBLE), 1)
	BUILD_DIR := build/$(PLATFORM)_$(TARGET_OS)_$(TARGET)_CU$(CU)_VF$(VF)_double
else
	BUILD_DIR := build/$(PLATFORM)_$(TARGET_OS)_$(TARGET)_CU$(CU)_VF$(VF)_float
endif

# Source Files
SRC_DIR := src
OBJECTS += \
$(pwd)/$(BUILD_DIR)/main.o \
$(pwd)/$(BUILD_DIR)/util.o \
$(pwd)/$(BUILD_DIR)/csr.o \
$(pwd)/$(BUILD_DIR)/csr_hw.o \
$(pwd)/$(BUILD_DIR)/csr_hw_wrapper.o \
$(pwd)/$(BUILD_DIR)/spmv.o

# SDS Options
HW_FLAGS :=
HW_FLAGS += -sds-hw spmv spmv.cpp -clkid 2 -sds-end

EMU_FLAGS := 
ifeq ($(TARGET), emu)
	EMU_FLAGS := -mno-bitstream -mno-boot-files -emulation $(EMU_MODE)
endif

# Compilation and Link Flags
IFLAGS := -I.
CFLAGS = -Wall -O3 -c
CFLAGS += -MT"$@" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" 
CFLAGS += $(ADDL_FLAGS)
CFLAGS += -D"CU=$(CU)" -D"VF=$(VF)" -D"DOUBLE=$(DOUBLE)"
LFLAGS = "$@" "$<" 

SDSFLAGS := -sds-pf $(PLATFORM) \
		    -target-os $(TARGET_OS)
ifeq ($(VERBOSE), 1)
	SDSFLAGS += -verbose 
endif

# SDS Compiler
CC := sds++ $(SDSFLAGS)

.PHONY: all
all: $(BUILD_DIR)/$(EXECUTABLE)

$(BUILD_DIR)/$(EXECUTABLE): $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	@echo 'Building Target: $@'
	@echo 'Trigerring: SDS++ Linker'
	cd $(BUILD_DIR) ; $(CC) -o $(EXECUTABLE) $(OBJECTS) $(EMU_FLAGS)
	@echo 'SDx Completed Building Target: $@'
	@echo ' '

$(pwd)/$(BUILD_DIR)/%.o: $(pwd)/$(SRC_DIR)/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: SDS++ Compiler'
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) ; $(CC) $(CFLAGS) -o $(LFLAGS) $(HW_FLAGS)
	@echo 'Finished building: $<'
	@echo ' '

# Check Rule Builds the Sources and Executes on Specified Target
check: all
ifeq ($(TARGET), emu)

    ifneq ($(TARGET_OS), linux)
	    cd $(BUILD_DIR) ; sdsoc_emulator -timeout 300
    endif

else
	$(info "This Release Doesn't Support Automated Hardware Execution")
endif

.PHONY: cleanall clean ultraclean
clean:
	$(RM) $(BUILD_DIR)/$(EXECUTABLE) $(OBJECTS)
cleanall:clean
	$(RM) -rf $(BUILD_DIR) .Xil

ECHO:= @echo

.PHONY: help
help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "	make all TARGET=<emu/hw> TARGET_OS=<linux/standalone> CU=<1/2/4/8/10/12> VF=<1/2/4/8> DOUBLE=<0/1>" 
	$(ECHO) "		Command to generate the design for specified Target and OS."
	$(ECHO) ""
