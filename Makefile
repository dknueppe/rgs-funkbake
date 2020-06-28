BUILD_DIR = build
TARGET = funkbake
SRCS = funkbake.c
INCS = 

EXTENDED_FUSE   = 0xFF
HIGH_FUSE       = 0xDF
LOW_FUSE        = 0x7F

# Change Port accordingly
PROG_PORT       = /dev/ttyACM0
MICROCONTROLLER = attiny84
PROGRAMMER      = stk500
FORMAT          = ihex

CC 	            = avr-gcc
OBJCOPY         = avr-objcopy
OPTIMIZATION    = s
FLAGS           = unsigned-char unsigned-bitfields function-sections data-sections \
                  short-enums pack-struct strict-aliasing
WNO_ERR	        = unused-variable unused-function unused-value unused-label \
                  missing-prototypes missing-declarations missing-include-dirs
WARNINGS        = all extra pedantic error cast-align cast-qual disabled-optimization \
                  format=2 logical-op missing-declarations missing-include-dirs shadow \
                  redundant-decls sign-conversion strict-overflow=5 switch-default undef \
                  maybe-uninitialized pointer-arith missing-prototypes
CFLAGS          = -mmcu=$(MICROCONTROLLER) -g$(DEBUG) -O$(OPTIMIZATION) -std=c11

all : $(BUILD_DIR) $(BUILD_DIR)/$(TARGET).hex program

$(BUILD_DIR) :
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET).elf : $(BUILD_DIR) $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(BUILD_DIR)/$(TARGET).elf \
	$(foreach I, $(INCS), -I$I) \
	$(foreach W, $(WARNINGS), -W$W) \
	$(foreach F, $(FLAGS), -f$F) \
	$(foreach W, $(WNO_ERR), -Wno-error=$W)

%.hex : %.elf
	$(OBJCOPY) -O $(FORMAT) -R .eeprom $^ $@

fuses :
	avrdude -c $(PROGRAMMER) -P $(PROG_PORT) -p $(MICROCONTROLLER) -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:$(HIGH_FUSE):m -U efuse:w:$(EXTENDED_FUSE):m

program: $(BUILD_DIR)/$(TARGET).hex
	avrdude -p $(MICROCONTROLLER) -P $(PROG_PORT) -c $(PROGRAMMER) -U flash:w:$^

doc_$(TARGET).pdf : README.md
	pandoc -o $@ $^ -f markdown-implicit_figures

doc : doc_$(TARGET).pdf

clean :
	rm -rf $(BUILD_DIR)

.PHONY : all program clean doc
