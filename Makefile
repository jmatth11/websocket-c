CC=clang
CFLAGS=-Wall -std=c11 -lm
INCLUDES=-I. -I./deps/cstd/headers -I./deps/cstd/deps/utf8-zig/headers/
LIBS=-L./deps/cstd/lib -L./deps/cstd/deps/utf8-zig/zig-out/lib/ -lcustom_std -lutf8-zig
SOURCES=$(shell find . -name '*.c' -not -path './plugins/*' -not -path './deps/*' -not -path './libs/*' -not -path './tests/*')
OBJECTS=$(addprefix $(OBJ)/,$(SOURCES:%.c=%.o))

BIN=bin
OBJ=obj
TARGET=main
SHARED=libws.so
ARCHIVE=libws.a

.PHONY: all
all: $(OBJECTS)
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(LIBS) $^ -o $(BIN)/$(TARGET)

$(OBJ)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c -fPIC -o $@ $< $(CFLAGS) $(INCLUDES)

.PHONY: archive
archive: $(filter-out %main.o,$(OBJECTS))
	@mkdir -p $(BIN)
	$(CC) -shared -fPIC -o $(BIN)/$(SHARED) $^ $(LIBS)
	ar -rcs $(BIN)/$(ARCHIVE) $^

.PHONY: clean
clean:
	@rm -rf $(BIN)/*
	@rm -rf $(OBJ)/*
