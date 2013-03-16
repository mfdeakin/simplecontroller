#
#Usage: Set ARDDIR to the directory containing the github repository found here:
#https://github.com/arduino/Arduino
#There are some other steps to set the repository up for building for the Due, but I won't cover them here.
#
#To program the Due, erase the flash, then run "make upload",
#specifying the port on which the Arduino is running as PORT
#
#Example:
#make upload PORT=/dev/ttyACM0
#Your Arduino may exist on another port, like ttyUSB0
#

ARDDIR=/home/michael/Documents/Programming/arduino/Arduino
SAMDIR=$(ARDDIR)/build/linux/work/hardware/arduino/sam
SYSDIR=$(SAMDIR)/system
CC=$(ARDDIR)/build/linux/work/hardware/tools/g++_arm_none_eabi/bin/arm-none-eabi-gcc
CXX=$(ARDDIR)/build/linux/work/hardware/tools/g++_arm_none_eabi/bin/arm-none-eabi-g++
CXXAR=$(ARDDIR)/build/linux/work/hardware/tools/g++_arm_none_eabi/bin/arm-none-eabi-ar
CXXOBJCOPY=$(ARDDIR)/build/linux/work/hardware/tools/g++_arm_none_eabi/bin/arm-none-eabi-objcopy
UPLOAD=$(ARDDIR)/build/linux/work/hardware/tools/bossac
UPLOADOPTS=-U false -e -w -v -b
OBJECTOUTDIR=objects
INCDIRS=-I$(SYSDIR)/libsam -I$(SYSDIR)/CMSIS/CMSIS/Include/ -I$(SYSDIR)/CMSIS/Device/ATMEL/ -I$(SAMDIR)/cores/arduino -I$(SAMDIR)/variants/arduino_due_x
CFLAGS=-g -Os -w -ffunction-sections -fdata-sections -nostdlib --param max-inline-insns-single=500 -Dprintf=iprintf -mcpu=cortex-m3 -DF_CPU=84000000L -DARDUINO=152 -D__SAM3X8E__ -mthumb -DUSB_PID=0x003e -DUSB_VID=0x2341 -DUSBCON
CXXFLAGS=-g -Os -w -ffunction-sections -fdata-sections -nostdlib --param max-inline-insns-single=500 -fno-rtti -fno-exceptions -Dprintf=iprintf -mcpu=cortex-m3 -DF_CPU=84000000L -DARDUINO=152 -D__SAM3X8E__ -mthumb -DUSB_PID=0x003e -DUSB_VID=0x2341 -DUSBCON
LINKFLAGS=-Os -Wl,--gc-sections -mcpu=cortex-m3 -T/home/michael/Documents/Programming/arduino/Arduino/build/linux/work/hardware/arduino/sam/variants/arduino_due_x/linker_scripts/gcc/flash.ld -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--entry=Reset_Handler -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align -Wl,--warn-unresolved-symbols 

OBJECTS=simple.o scheduler.o modem.o motor.o list.o heap.o ieeehalfprecision.o

$(OBJECTOUTDIR)/program.cpp.bin: $(OBJECTS)
	@echo "Linking program"
	@$(CXX) $(LINKFLAGS) -Wl,-Map,$(OBJECTOUTDIR)/program.cpp.map -o $(OBJECTOUTDIR)/program.cpp.elf -lm -lgcc -mthumb -Wl,--start-group $(OBJECTOUTDIR)/syscalls_sam3.c.o $(OBJECTS) $(SAMDIR)/variants/arduino_due_x/libsam_sam3x8e_gcc_rel.a $(OBJECTOUTDIR)/core.a -Wl,--end-group 
	@echo "Creating binary"
	@$(CXXOBJCOPY) -O binary $(OBJECTOUTDIR)/program.cpp.elf $(OBJECTOUTDIR)/program.cpp.bin

#Open and close a serial connection to the Arduino at 1200 baud
#to erase the memory of the microprocessor
upload: $(OBJECTOUTDIR)/program.cpp.bin
	@echo "Uploading program"
	@./erase.py $(PORT)
	@$(UPLOAD) --port=$(PORT) $(UPLOADOPTS) $(OBJECTOUTDIR)/program.cpp.bin -R

%.o: %.cpp
	@echo "Compiling $@"
	@$(CXX) $(CXXFLAGS) $(INCDIRS) -c -o $@ $<

%.o: %.c
	@echo "Compiling $@"
	@$(CC) $(CXXFLAGS) $(INCDIRS) -c -o $@ $<

core.a:
	@mkdir $(OBJECTOUTDIR) > /dev/null 2>&1; true
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/hooks.c -o $(OBJECTOUTDIR)/hooks.c.o
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/wiring.c -o $(OBJECTOUTDIR)/wiring.c.o
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/iar_calls_sam3.c -o $(OBJECTOUTDIR)/iar_calls_sam3.c.o
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/WInterrupts.c -o $(OBJECTOUTDIR)/WInterrupts.c.o
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/syscalls_sam3.c -o $(OBJECTOUTDIR)/syscalls_sam3.c.o
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/wiring_digital.c -o $(OBJECTOUTDIR)/wiring_digital.c.o
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/cortex_handlers.c -o $(OBJECTOUTDIR)/cortex_handlers.c.o
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/wiring_shift.c -o $(OBJECTOUTDIR)/wiring_shift.c.o
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/itoa.c -o $(OBJECTOUTDIR)/itoa.c.o
	@$(CC) -c $(CFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/wiring_analog.c -o $(OBJECTOUTDIR)/wiring_analog.c.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/RingBuffer.cpp -o $(OBJECTOUTDIR)/RingBuffer.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/IPAddress.cpp -o $(OBJECTOUTDIR)/IPAddress.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/UARTClass.cpp -o $(OBJECTOUTDIR)/UARTClass.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/USB/USBCore.cpp -o $(OBJECTOUTDIR)/USBCore.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/USB/HID.cpp -o $(OBJECTOUTDIR)/HID.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/USB/CDC.cpp -o $(OBJECTOUTDIR)/CDC.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/Stream.cpp -o $(OBJECTOUTDIR)/Stream.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/Reset.cpp -o $(OBJECTOUTDIR)/Reset.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/WMath.cpp -o $(OBJECTOUTDIR)/WMath.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/cxxabi-compat.cpp -o $(OBJECTOUTDIR)/cxxabi-compat.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/wiring_pulse.cpp -o $(OBJECTOUTDIR)/wiring_pulse.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/Print.cpp -o $(OBJECTOUTDIR)/Print.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/WString.cpp -o $(OBJECTOUTDIR)/WString.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/main.cpp -o $(OBJECTOUTDIR)/main.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/cores/arduino/USARTClass.cpp -o $(OBJECTOUTDIR)/USARTClass.cpp.o
	@$(CXX) -c $(CXXFLAGS) $(INCDIRS) $(SAMDIR)/variants/arduino_due_x/variant.cpp -o $(OBJECTOUTDIR)/variant.cpp.o

	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/hooks.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/wiring.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/iar_calls_sam3.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/WInterrupts.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/syscalls_sam3.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/wiring_digital.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/cortex_handlers.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/wiring_shift.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/itoa.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/wiring_analog.c.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/RingBuffer.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/IPAddress.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/UARTClass.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/USBCore.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/HID.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/CDC.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/Stream.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/Reset.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/WMath.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/cxxabi-compat.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/wiring_pulse.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/Print.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/WString.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/main.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/USARTClass.cpp.o
	@$(CXXAR) rcs $(OBJECTOUTDIR)/core.a $(OBJECTOUTDIR)/variant.cpp.o
