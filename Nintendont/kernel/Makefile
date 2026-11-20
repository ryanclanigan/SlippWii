# Nintendont kernel

#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------

# Grab some info from git to make versioning somewhat easier.
GIT_VERSION := "$(shell git describe --dirty --always --tags)"

.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

PREFIX	:= $(DEVKITARM)/bin/arm-none-eabi-
CC	:= $(PREFIX)gcc
AS	:= $(PREFIX)as
LD	:= $(PREFIX)gcc
OBJCOPY	:= $(PREFIX)objcopy

ifeq ($(windows), 1)
	ZIP = zip/zip.exe
else
	ZIP = zip
endif


CFLAGS	:= -mbig-endian -Os -std=gnu89 -g -I. -Wall \
	   -mcpu=arm926ej-s -msoft-float -mfloat-abi=soft \
	   -fdata-sections -ffunction-sections -fno-builtin-toupper -fno-builtin-memcpy \
	   -fno-builtin-memset -fno-builtin-memcmp -fno-builtin-malloc -fno-builtin-free \
	   -DNIN_GIT_VERSION=\"$(GIT_VERSION)\"
CPPFLAGS := -I. -I../fatfs -I../codehandler

ASFLAGS	:= -mbig-endian -mcpu=arm926ej-s
LDFLAGS	:= -flto -Os -nostartfiles -nodefaultlibs -mbig-endian -Wl,--gc-sections,-T,kernel.ld,-Map,kernel.map -n

TARGET	:= kernel.elf
OBJECTS	:= start.o common.o alloc.o GCAM.o JVSIO.o JVSIOMessage.o FST.o DI.o RealDI.o \
	   SlippiMemory.o SlippiFileWriter.o SlippiCommunication.o SlippiNetwork.o \
	   SlippiNetworkBroadcast.o Patch.o PatchTimers.o TRI.o \
	   PatchWidescreen.o ISO.o Stream.o adp.o EXI.o SRAM.o GCNCard.o SI.o HID.o diskio.o \
	   Config.o utils_asm.o ES.o NAND.o main.o syscalls.o ReadSpeed.o vsprintf.o string.o \
	   prs.o SDI.o usb.o usbstorage.o wdvd.o net.o
ASIX_CORE := $(wildcard asix/lwip/src/core/*.c) $(wildcard asix/lwip/src/core/ipv4/*.c) asix/lwip/src/netif/ethernet.c
OBJECTS += asix/asix.o asix/hardware.o $(ASIX_CORE:.c=.o)
# Compile for size: rollback plus USB Ethernet must fit the fixed code region.
# The PPC game/rollback code retains its own optimization flags.
# Pure C helpers use ARM/Thumb interworking; IOS thread entry points stay ARM.
vsprintf.o: CFLAGS += -mthumb
asix/%.o: asix/%.c
	@echo "CC    $<"
	@$(CC) $(CFLAGS) -std=gnu11 -Os -mthumb -flto -Iasix/port -Iasix/lwip/src/include $(CPPFLAGS) -c -MMD -MP -MF $(@:.o=.d) -o $@ $<

# (BearSSL for slippi_online/so_tls.c: tools/build_bearssl.py --arm --install)
LIBS	:= ../fatfs/libfatfs-arm.a extlibs/lib/libbearssl-arm.a be/libc.a be/libgcc.a
ZIPFILE	:= ../loader/data/kernel.zip

# Bluetooth stack
OBJECTS	+= lwbt/bte.o lwbt/btmemb.o lwbt/btmemr.o lwbt/btpbuf.o lwbt/hci.o lwbt/l2cap.o lwbt/physbusif.o BT.o

# Slippi Online: matchmaking and netplay (slippi_online/README.md), with
# ENet 1.3.13 on an IOS platform layer
SO_SOURCES := $(wildcard slippi_online/*.c) $(wildcard slippi_online/enet/*.c)
OBJECTS += $(SO_SOURCES:.c=.o)
SO_CFLAGS := -std=gnu99 -Os -flto -DENET_IOS -Islippi_online/enet/include -I../loader/extlibs/include

# Keep the linker-script TLS read-only section placement visible to the linker.
slippi_online/so_tls.o slippi_online/so_ws.o slippi_online/so_ws_frame.o slippi_online/so_mm_ws.o slippi_online/so_tcp.o slippi_online/so_stun.o slippi_online/so_stun_io.o: SO_CFLAGS += -fno-lto

slippi_online/%.o : slippi_online/%.c
	@echo  "CC	$@"
	@$(CC) $(CFLAGS) $(SO_CFLAGS) $(CPPFLAGS) -c -MMD -MP -MF $(@:.o=.d) -o $@ $<

# UBJSON Implementation
OBJECTS += ubj/ubjr.o ubj/ubjrw.o ubj/ubjw.o

# Common configs
OBJECTS += ../common/config/MeleeCodes_arm.o

# SlippWii: the kernel's code region (kernel.ld: 384 KB, with the online boot codes in it) is full.
# The Triforce arcade boards' code, which Melee never runs, is built for size, not speed.
GCAM.o JVSIO.o JVSIOMessage.o TRI.o: CFLAGS += -Os
# SlippWii: so is the patcher of the game's timers, which runs once as the game loads (12 KB at -O3,
# the code region's last room: a test build's kernel was 40 bytes over it, 2026-11-15).
PatchTimers.o: CFLAGS += -Os
# (and Patch.o, for room in the code region: it patches the game at its start, nothing in play;
# input suggestions for Slippi 3.7.0 left it 1.8 KB short at -O3, 2026-11-17)
Patch.o: CFLAGS += -Os

# When make is invoked with `make SLIPPI_DEBUG=1`: compile SlippiDebug.o and
# let GCC define SLIPPI_PROFILE as a macro. Use this for some profiling code.

ifeq ($(SLIPPI_DEBUG), 1)
	OBJECTS += SlippiDebug.o
	CFLAGS += -DSLIPPI_DEBUG
endif

#---------------------------------------------------------------------------------
# For pulling in built-in Gecko codes
ifeq ($(OS),Windows_NT)
	BIN2H 	= ./bin2h/bin2h.exe
else
	BIN2H 	= ./bin2h/bin2h
endif

GECKO_DIR 	:= gecko
GECKO		:= $(wildcard gecko/*.bin)
GECKO_H		:= $(GECKO:%.bin=%.h)

$(GECKO_DIR)/%.h: $(GECKO_DIR)/%.bin
	@echo "BIN2H    $<"
	@$(BIN2H) $<

#---------------------------------------------------------------------------------
# For building all common config files
COMMON_CFG_DIR	:= ../common/config
COMMON_CFG_DEPS := $(wildcard ../common/config/*.c)
COMMON_CFG	:= $(COMMON_CFG_DEPS:%.c=%_arm.o)

$(COMMON_CFG_DIR)/%_arm.o: ${COMMON_CFG_DIR}/%.c
	@echo "CC    $<"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MP -MF $(@:.o=.d) \
		-o $@ $<


#---------------------------------------------------------------------------------
.PHONY: FORCE
all: $(TARGET)

-include $(OBJECTS:.o=.d)

$(TARGET): $(GECKO_H) $(COMMON_CFG) kernel.ld $(OBJECTS)
	@echo  "LD	$@"
	@$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $@
	@$(OBJCOPY) -S -O binary kernel.elf kernel.bin
	@echo  "ZIP	$(ZIPFILE)"
	@$(ZIP) -JXjq9 $(ZIPFILE) kernel.bin

%.o: %.s
	@echo  "AS	$@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -D_LANGUAGE_ASSEMBLY -c -MMD -MP -MF $(@:.o=.d) -x assembler-with-cpp -o $@ $<

%.o: %.S
	@echo  "AS	$@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -D_LANGUAGE_ASSEMBLY -c -MMD -MP -MF $(@:.o=.d) -x assembler-with-cpp -o $@ $<

%.o : %.c
	@echo  "CC	$@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MP -MF $(@:.o=.d) -o $@ $<

clean:
	-$(RM) -f $(ZIPFILE) *.elf *.o *.bin *.d *.map lwbt/*.d lwbt/*.o option/*.d option/*.o gecko/*.h \
		../common/config/*.o ../common/config/*.d ubj/*.o ubj/*.d \
		slippi_online/*.o $(ASIX_CORE:.c=.o) $(ASIX_CORE:.c=.d) asix/*.o asix/*.d slippi_online/*.d slippi_online/enet/*.o slippi_online/enet/*.d
