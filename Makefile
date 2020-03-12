BUILD_DIR = build
TARGET = funkbake
SRCS = funkbake.c
INCS = 

# Change Port accordingly
PROG_PORT = /dev/ttyACM0
MICROCONTROLLER = attiny84
PROGRAMMER = stk500
FORMAT = ihex

CC = avr-gcc
OBJCOPY = avr-objcopy

DEBUG = dwarf-2
OPTIMIZATION = s

FLAGS	 = unsigned-char unsigned-bitfields function-sections data-sections \
           short-enums pack-struct
WARNINGS = all extra pedantic error cast-align cast-qual disabled-optimization \
		   format=2 logical-op missing-declarations missing-include-dirs shadow \
		   redundant-decls sign-conversion strict-overflow=5 switch-default undef \
		   maybe-uninitialized pointer-arith 
CCFLAGS  = -mmcu=$(MICROCONTROLLER) -g$(DEBUG) -O$(OPTIMIZATION) -std=c11

all : $(BUILD_DIR) $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR) :
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET).elf : $(BUILD_DIR) $(SRCS)
	$(CC) $(CCFLAGS) $(SRCS) -o $(BUILD_DIR)/$(TARGET).elf \
	$(foreach I, $(INCS), -I$I) \
	$(foreach W, $(WARNINGS), -W$W) \
	$(foreach F, $(FLAGS), -f$F)

%.hex : %.elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $^ $@

program: $(BUILD_DIR)/$(TARGET).hex
	avrdude -p $(MICROCONTROLLER) -P $(PROG_PORT) -c $(PROGRAMMER) -U flash:w:$^

clean :
	rm -rf $(BUILD_DIR)

.PHONY : all program clean
