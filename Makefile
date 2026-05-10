CC = gcc
TARGET = ttt_laptop
SRC = ttt_laptop.c

RAYLIB_PATH = $(shell brew --prefix raylib)
MOSQ_PATH   = $(shell brew --prefix mosquitto)
CJSON_PATH  = $(shell brew --prefix cjson)

CFLAGS = -Wall -Wextra -std=c11 \
         -I$(RAYLIB_PATH)/include \
         -I$(MOSQ_PATH)/include \
         -I$(CJSON_PATH)/include

LDFLAGS = -L$(RAYLIB_PATH)/lib \
          -L$(MOSQ_PATH)/lib \
          -L$(CJSON_PATH)/lib

LIBS = -lraylib -lmosquitto -lcjson \
       -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)