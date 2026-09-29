AS      = as
LD      = ld
AR      = ar
ASFLAGS =
LDFLAGS = -static -no-pie

BUILD   = build
SRCS    = $(wildcard *.s)
LIBSRCS = $(filter-out main.s,$(SRCS))
LIBOBJS = $(patsubst %.s,$(BUILD)/%.o,$(LIBSRCS))
MAINOBJ = $(BUILD)/main.o
LIB     = $(BUILD)/libasm.a
TARGET  = $(BUILD)/main.bin

# COMMAND RUNNERS
.PHONY: all run debug clean
run: $(TARGET)
	./$(TARGET)

all: run

debug: $(TARGET)
	gdb ./$(TARGET) -ex "layout asm" -ex "layout reg" -ex "starti"

clean:
	rm -rf $(BUILD)

# BUILDING
$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.s | $(BUILD)
	@$(AS) $(ASFLAGS) -o $@ $< 

$(LIB): $(LIBOBJS)
	$(AR) rcs $@ $(LIBOBJS)

$(TARGET): $(MAINOBJ) $(LIB)
	$(LD) $(LDFLAGS) -o $@ $(MAINOBJ) $(LIB)
