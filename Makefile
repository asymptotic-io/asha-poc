# Comment to disable debug logs & symbols
#
# Many thanks to https://stackoverflow.com/a/30602701
-include $(OBJ:.o=.d)

SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
EXE := $(BIN_DIR)/asha-support
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

CFLAGS+=-DDEBUG -g
CFLAGS+=-Iinclude
CFLAGS+=-g -Wall -Wextra
CFLAGS+=$(shell pkg-config --libs --cflags dbus-1)

LDFLAGS=$(shell pkg-config --libs dbus-1)
LDFLAGS=-lpthread

.PHONY: all

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $@

clean:
	rm bin/asha-support
