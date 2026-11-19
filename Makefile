#
# ReflexBoard - reaction-time trainer for the DE10-Standard HPS
#
# Usage: make help
#
TARGET = reflexboard
SIM    = reflexboard-sim

ALT_DEVICE_FAMILY ?= soc_cv_av
SOCEDS_ROOT ?= $(SOCEDS_DEST_ROOT)
HWLIBS_ROOT = $(SOCEDS_ROOT)/ip/altera/hps/altera_hps/hwlib
CROSS_COMPILE = arm-linux-gnueabihf-
CC = $(CROSS_COMPILE)gcc
HOSTCC ?= gcc

INCLUDES = -D$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/$(ALT_DEVICE_FAMILY) -I$(HWLIBS_ROOT)/include/ \
           -Isrc -Ilib/terasic_lcd
# -O0: keeps every variable visible in gdb (make debug)
CFLAGS = -g -O0 -Wall $(INCLUDES)
LDLIBS = -lrt -lm

# Game code + LCD drawing: shared by the board and the simulator
COMMON_SRC = src/main.c src/screen.c lib/terasic_lcd/lcd_graphic.c lib/terasic_lcd/font.c
# Real hardware: GPIO/LCD through /dev/mem + Terasic LCD driver (SPI)
BOARD_SRC  = $(COMMON_SRC) src/board.c \
             lib/terasic_lcd/LCD_Hw.c lib/terasic_lcd/LCD_Driver.c lib/terasic_lcd/LCD_Lib.c
# PC simulator: keyboard = button, LCD frames saved as images
SIM_SRC    = $(COMMON_SRC) src/board_sim.c
HEADERS    = $(wildcard src/*.h lib/terasic_lcd/*.h)

# ---------------------------------------------------------------------------
# Target (DE10-Standard) configuration - override on the command line,
# e.g.: make run TARGET_IP=192.168.0.42
# ---------------------------------------------------------------------------
TARGET_IP   ?= 192.168.10.13
TARGET_USER ?= root
TARGET_DIR  ?= /home/root
GDB_PORT    ?= 2345
GDB         ?= gdb-multiarch

# The board's sshd is old: accept its ssh-rsa host key, and don't pin the host
# key (it changes every time the SD card is re-flashed).
SSH_OPTS = -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR \
           -o HostKeyAlgorithms=+ssh-rsa -o PubkeyAcceptedAlgorithms=+ssh-rsa
SSH = ssh $(SSH_OPTS) $(TARGET_USER)@$(TARGET_IP)
# -O: legacy scp protocol, the target may not have an sftp-server
SCP = scp -O $(SSH_OPTS)
REMOTE_BIN = $(TARGET_DIR)/$(TARGET)

build: $(TARGET)

# Cross-compile for the board (ARM)
$(TARGET): $(BOARD_SRC) $(HEADERS)
	$(CC) $(CFLAGS) $(BOARD_SRC) -o $@ $(LDLIBS)

# Compile for this PC (x86) and play with the keyboard
$(SIM): $(SIM_SRC) $(HEADERS)
	$(HOSTCC) $(CFLAGS) -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast $(SIM_SRC) -o $@ $(LDLIBS)

sim: $(SIM)
	./$(SIM)

# Copy the binary to the target
deploy: $(TARGET)
	$(SCP) $(TARGET) $(TARGET_USER)@$(TARGET_IP):$(REMOTE_BIN)
	$(SSH) "chmod +x $(REMOTE_BIN) && sync"

# Deploy and play on the target (Ctrl+C stops it)
run: deploy
	$(SSH) -t "cd $(TARGET_DIR) && ./$(TARGET)"

# Open a shell on the target
ssh:
	$(SSH)

# Install your public key on the target so deploy/run don't ask for a password
ssh-key:
	ssh-copy-id $(SSH_OPTS) $(TARGET_USER)@$(TARGET_IP)

# Start gdbserver on the target and attach gdb from the host (stops at main)
debug: deploy
	$(SSH) -n "killall gdbserver 2>/dev/null; gdbserver :$(GDB_PORT) $(REMOTE_BIN)" & \
	sleep 2; \
	$(GDB) -q $(TARGET) \
		-ex "target remote $(TARGET_IP):$(GDB_PORT)" \
		-ex "break main" \
		-ex "continue"

help:
	@echo "make            - cross-compile $(TARGET) for the board"
	@echo "make sim        - build and play the PC simulator (any key = USER BUTTON)"
	@echo "make deploy     - copy $(TARGET) to $(TARGET_USER)@$(TARGET_IP):$(TARGET_DIR)"
	@echo "make run        - deploy and play on the board"
	@echo "make debug      - deploy, start gdbserver on the board and attach $(GDB)"
	@echo "make ssh        - open a shell on the board"
	@echo "make ssh-key    - install your ssh key on the board (run once)"
	@echo "make clean      - remove build files"
	@echo "Variables: TARGET_IP=$(TARGET_IP) TARGET_USER=$(TARGET_USER) TARGET_DIR=$(TARGET_DIR) GDB_PORT=$(GDB_PORT)"

.PHONY: build sim deploy run ssh ssh-key debug help clean
clean:
	rm -f $(TARGET) $(SIM) *.o
