CC=clang
CFLAGS=-Wall -Wextra -std=gnu11
INCLUDES=-I. -I./deps/cstd/headers -I./deps/cstd/deps/utf8-zig/headers/
LIBS=-L./deps/cstd/lib -L./deps/cstd/deps/utf8-zig/zig-out/lib/ -lcustom_std -lutf8-zig
SOURCES=$(shell find . -name '*.c' -not -path './plugins/*' -not -path './deps/*' -not -path './libs/*' -not -path './tests/*')
TARGET=main
DFLAGS=-DAPP_HASH=$(shell git log -n 1 --pretty=format:"%H")

ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG=1 -ggdb
endif
ifeq ($(RELEASE), 1)
	CFLAGS += -O2
else
	CFLAGS += -fsanitize=undefined -fsanitize-trap=undefined
	LIBS += -lubsan
endif
ifeq ($(SHARED), 1)
	SOURCES=$(shell find ./src -name '*.c')
	TARGET=libws
endif

ifeq ($(USE_SSL), 1)
	LIBS += $(shell pkg-config --libs openssl)
	DFLAGS += -DWEBC_USE_SSL=1
endif
ifeq ($(DISABLE_SIMD), 1)
	DFLAGS += -DDISABLE_SIMD=1
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
	$(CC) $^ -o $(BIN)/$(TARGET) $(CFLAGS) $(LIBS)
endif

$(OBJ)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c -fPIC -o $@ $< $(CFLAGS) $(INCLUDES) $(DFLAGS)

.PHONY: clean
clean:
	@rm -rf $(BIN)/*
	@rm -rf $(OBJ)/*
