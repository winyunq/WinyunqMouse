#	生成hex文件目标 TARGET.hex
TARGET = Winyunq
#	riscv编译链路径
ToolPath = 'C:\MounRiver\MounRiver_Studio\toolchain\RISC-V Embedded GCC\bin\'
#临时文件路径
BUILD_DIR = bin
#	C源文件，添加格式:
#	C_SOURCES += $(wildcard <C文件路径>/*.c )
C_SOURCES += $(wildcard src/*.c ) 
C_SOURCES += $(wildcard libs/HAL/*.c )
C_SOURCES += $(wildcard libs/Profile/*.c )
C_SOURCES += $(wildcard libs/StdPeriphDriver/*.c )
#	头文件，添加格式:
#	-I<头文件路径> \ 
C_INCLUDES =  \
-Isrc/include \
-Ilibs/HAL/include \
-Ilibs/LIB \
-Ilibs/Profile/include \
-Ilibs/RVMSIS \
-Ilibs/StdPeriphDriver/inc \
#	启动文件
ASM_SOURCES =  \
libs/Startup/startup_CH573.s
#	链接文件
LDSCRIPT = libs/Ld/Link.ld
#	蓝牙库
LIBS = -lISP573 -lCH57xBLE
LIBDIR = \
-Llibs/LIB \
-Llibs/StdPeriphDriver
# 全局性质头文件
CONFIGURES = -include src/include/Configure.h
#	GCC编译链，可修改 ToolPath 的值指定GCC编译链路径，同时可修改 riscv-none-embed- 前缀，指定不同目标
PREFIX = $(ToolPath)riscv-none-embed-
CC = $(PREFIX)gcc
CPP = $(PREFIX)g++
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
LD = $(PREFIX)LD
HEX = $(PREFIX)objcopy -O ihex 

ConfigureFlag = -DDEBUG=0 -DCLK_OSC32K=2 -DDCDC_ENABLE=0 -DHAL_SlEEP=1 -DBLE_TX_POWER=0x25

CFLAGS = -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common $(ConfigureFlag) -g -Wall $(C_INCLUDES)
ASFLAGS = -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -x assembler
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
ASFLAGS += -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
#	riscv32 imac架构
LDFLAGS = -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -g -T
LDFLAGS += $(LDSCRIPT) -nostartfiles -Xlinker --gc-sections $(LIBDIR) -Xlinker --print-memory-usage -Wl,-Map=$(BUILD_DIR)/$(TARGET).map --specs=nano.specs --specs=nosys.specs -o $(BUILD_DIR)/$(TARGET).elf
LDFLAGS_Release = -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -T $(LDSCRIPT) -nostartfiles -Xlinker --gc-sections $(LIBDIR) -Xlinker --print-memory-usage -Wl,-Map=$(BUILD_DIR)/$(TARGET).map --specs=nano.specs --specs=nosys.specs
#	支持多线程编译，make -j<线程数>
all: $(TARGET).elf

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.S $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) $(CFLAGS) $(CONFIGURES)

$(BUILD_DIR)/%.o: %.S Makefile | $(BUILD_DIR)
	$(CC) $(ASFLAGS)

$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS)
	$(SZ) $(BUILD_DIR)/$(TARGET).elf
	$(PREFIX)objcopy -O ihex $(BUILD_DIR)/$(TARGET).elf $(TARGET).hex

release:
ifneq ($(wildcard $(BUILD_DIR)),)
	rm -rf $(BUILD_DIR)
endif
	mkdir $(BUILD_DIR)	
	$(CC) $(C_SOURCES) $(CONFIGURES) $(ASM_SOURCES) $(C_INCLUDES) $(LIBS) $(LDFLAGS_Release) -o $(BUILD_DIR)/$(TARGET).elf
	$(PREFIX)objcopy -O ihex $(BUILD_DIR)/$(TARGET).elf $(TARGET).hex

clean:
ifneq ($(wildcard $(BUILD_DIR)),)
	rm -rf $(BUILD_DIR)
endif