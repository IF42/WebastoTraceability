CC=gcc
CFLAGS=-Wall -Wextra -pedantic -g -std=c2x -Ofast $$(pkg-config --cflags gtk+-3.0 sqlite3) -I/usr/include/ -Iutil
LIBS=$$(pkg-config --libs gtk+-3.0 sqlite3) -lcrypto -L. -lsnap7 -L/mingw64/lib -lpthread 

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
else
	LIBS+=-L/mingw64/lib
endif


TARGET=traceability
CACHE=.cache
OUTPUT=$(CACHE)/release

MODULES += main.o
MODULES += view.o
MODULES += ui.o
MODULES += controller.o
MODULES += plc_thread.o

MODULES += plc_thread1.o
MODULES += plc_thread2.o

MODULES += plc1_thread3.o
MODULES += plc1_thread4.o
MODULES += plc1_thread5.o
MODULES += plc1_thread6.o
MODULES += plc1_thread7.o
MODULES += plc1_thread8.o
MODULES += plc1_thread9.o
MODULES += plc1_thread10.o
MODULES += plc1_thread11.o
MODULES += plc1_thread12.o
MODULES += plc1_thread13.o
MODULES += plc1_thread14.o
MODULES += plc1_thread15.o
MODULES += plc1_thread16.o

MODULES += plc2_thread3.o
MODULES += plc2_thread4.o
MODULES += plc2_thread5.o
MODULES += plc2_thread6.o
MODULES += plc2_thread7.o
MODULES += plc2_thread8.o
MODULES += plc2_thread9.o

MODULES += model.o
MODULES += login.o

MODULES += endian.o
MODULES += array.o
MODULES += log.o
MODULES += util.o

TEST += test.o


OBJ=$(addprefix $(CACHE)/,$(MODULES))

T_OBJ=$(addprefix $(CACHE)/,$(TEST))

all: env $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o $(OUTPUT)/$(TARGET)


%.o:
	$(CC) $(CFLAGS) -c $< -o $@


-include dep.list


exec: all
	$(OUTPUT)/$(TARGET)


.PHONY: env dep clean


dep:
	$(CC) -Iutil -MM  app/*.c src/*.c util/*.c | sed 's|[a-zA-Z0-9_-]*\.o|$(CACHE)/&|' > dep.list


env:
	mkdir -pv $(CACHE)
	mkdir -pv $(OUTPUT)


clean: 
	rm -rvf $(OUTPUT)
	rm -vf $(CACHE)/*.o



