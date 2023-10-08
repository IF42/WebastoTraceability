CC=gcc
CFLAGS=-Wall -Wextra -pedantic -Ofast $$(pkg-config --cflags gtk+-3.0 sqlite3) -Iutil
LIBS=$$(pkg-config --libs gtk+-3.0 sqlite3) -lcrypto -L. -lsnap7 -lpthread 

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
else
	CFLAGS+= -mwindows
	LIBS+=-L/mingw64/lib
endif


TARGET=traceability
CACHE=.cache
OUTPUT=$(CACHE)/release

MODULES += main.o
MODULES += view.o
MODULES += ui.o
MODULES += controller.o
MODULES += model.o
MODULES += login.o
MODULES += plc_thread.o

MODULES += plc_ping.o
MODULES += plc_time_sync.o
MODULES += plc_sql_read_write.o
MODULES += plc_mes_csv.o
MODULES += plc_env_log.o

MODULES += endian.o
MODULES += log.o

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



