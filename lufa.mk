MPART      = uc3a1128
ARCH       = UC3
BOARD      = NONE
F_CPU      = 66000000
F_USB      = 96000000
C_STANDARD = gnu99
COPT       = -O2 -fdata-sections -ffunction-sections
LUFA_DEFS  = -DUSE_LUFA_CONFIG_HEADER -IConfig
             -DARCH=ARCH_$(ARCH) -DBOARD=BOARD_$(BOARD) \
             -DF_CPU=$(F_CPU)UL -DF_USB=$(F_USB)UL

# Path to the LUFA library, that's where you keep LUFA/ directory
LUFA_PATH = LUFA

include $(LUFA_PATH)/Build/lufa_sources.mk

SOURCES = $(LUFA_SRC_USB) $(LUFA_SRC_USBCLASS) $(LUFA_SRC_PLATFORM)

OBJECTS = $(SOURCES:.c=.o)
OBJECTS := $(OBJECTS:.S=.o)

CC = avr32-gcc

CFLAGS = -mpart=$(MPART) -x c $(COPT) -std=$(C_STANDARD) \
         -Wstrict-prototypes -masm-addr-pseudos \
         -Wall -fno-strict-aliasing -funsigned-char \
         -funsigned-bitfields
CFLAGS += $(LUFA_DEFS)

ASFLAGS = -mpart=$(MPART)

liblufa.a: $(OBJECTS)
	avr32-ar rv $@ $^
	avr32-ranlib $@

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -MF $(@:%.o=%.d) $<   -c -o $@

%.o: %.S
	$(CC) $(ASFLAGS) -MMD -MP -MF $(@:%.o=%.d) $<   -c -o $@

clean:
	rm -f $(OBJECTS) liblufa.a