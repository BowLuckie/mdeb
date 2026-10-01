CXX := g++
CC  := gcc

SRC_DIR  := src
LN_DIR   := vendor/linenoise
BUILD    := build
TARGET   := $(BUILD)/mdeb.bin

TEST_DIR := test
TEST_BIN := $(TEST_DIR)/raw_asm/build/main.bin

ELFIN_DIR := vendor/libelfin-fbreg
DWARF_A   := $(ELFIN_DIR)/dwarf/libdwarf++.a
ELF_A     := $(ELFIN_DIR)/elf/libelf++.a

ARGS ?= $(TEST_BIN)

CXXFLAGS := -std=c++17 -Wall -Wextra -g -I$(LN_DIR) \
            -I$(ELFIN_DIR) -I$(ELFIN_DIR)/elf -I$(ELFIN_DIR)/dwarf \
            -MMD -MP
CFLAGS   := -Wall -g -MMD -MP
LDFLAGS  :=
LDLIBS   := $(DWARF_A) $(ELF_A)

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD)/%.o) $(BUILD)/linenoise.o
DEPS := $(OBJS:.o=.d)

.DEFAULT_GOAL := all
.PHONY: all run clean rebuild test_bin

all: run

run: $(TARGET) test_bin
	./$(TARGET) $(ARGS)

rebuild: clean all

test_bin: $(BUILD)/linenoise.o
	$(MAKE) -C $(TEST_DIR) 

$(TARGET): $(OBJS) $(DWARF_A) $(ELF_A)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(DWARF_A) $(ELF_A) &:
	$(MAKE) -C $(ELFIN_DIR)

$(BUILD)/%.o: $(SRC_DIR)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/linenoise.o: $(LN_DIR)/linenoise.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	$(MAKE) -C $(TEST_DIR) clean
	$(MAKE) -C $(ELFIN_DIR) clean

-include $(DEPS)
