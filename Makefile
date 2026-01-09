CC=clang
CFLAGS=-Wall -Wextra -std=c11
INCLUDES=-I. -I./deps/cstd/headers -I./deps/cstd/deps/utf8-zig/headers/
LIBS=-L./deps/cstd/lib -L./deps/cstd/deps/utf8-zig/zig-out/lib/ -lcustom_std -lutf8-zig
SOURCES=$(shell find . -name '*.c' -not -path './plugins/*' -not -path './deps/*' -not -path './libs/*' -not -path './tests/*')
TARGET=main
DFLAGS=-DAPP_HASH=$(shell git log -n 1 --pretty=format:"%H")

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG=1 -ggdb -fsanitize=undefined -fsanitize-trap=undefined
endif
ifeq ($(RELEASE), 1)
	CFLAGS += -O2
endif
ifeq ($(SHARED), 1)
	SOURCES=$(shell find ./src -name '*.c')
	TARGET=libws
endif

ifeq ($(USE_SSL), 1)
	LIBS += $(shell pkg-config --libs libssl)
	DFLAGS += -DWEBC_USE_SSL=1
endif

OBJECTS=$(addprefix $(OBJ)/,$(SOURCES:%.c=%.o))

BIN=bin
OBJ=obj

.PHONY: all
all: $(OBJECTS)
	@mkdir -p $(BIN)
ifeq ($(SHARED), 1)
	$(CC) -shared -fPIC -o $(BIN)/$(TARGET).so $^ $(LIBS)
	ar -rcs $(BIN)/$(TARGET).a $^
else
	$(CC) $(CFLAGS) $(LIBS) $^ -o $(BIN)/$(TARGET)
endif

$(OBJ)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c -fPIC -o $@ $< $(CFLAGS) $(INCLUDES) $(DFLAGS)

.PHONY: clean
clean:
	@rm -rf $(BIN)/*
	@rm -rf $(OBJ)/*
