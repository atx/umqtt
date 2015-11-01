NAME = umqtt-esp
CC = xtensa-lx106-elf-gcc

UMQTT = ../umqtt

CFLAGS = -I$(UMQTT) -Wall -Wno-implicit-function-declaration -Os -I$(PWD) -mlongcalls -mlongcalls
LDLIBS = -nostdlib -Wl,--start-group -lmain -lupgrade -lnet80211 -lwpa -llwip \
		 -lpp -lphy -Wl,--end-group -lcirom -lgcc
LDFLAGS = -Teagle.app.v6.ld

$(NAME)-0x00000.bin: $(NAME)
	esptool.py elf2image $^

$(NAME): \
	main.o \
	driver/uart.o \
	$(UMQTT)/umqtt.c \
	umqtt-esp.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

flash: $(NAME)-0x00000.bin
	esptool.py write_flash 0x00000 $(NAME)-0x00000.bin 0x40000 $(NAME)-0x40000.bin

clean:
	rm -rf $(NAME) $(NAME)-0x00000.bin $(NAME)-0x40000.bin *.o driver/*.o $(UMQTT)/*.o

