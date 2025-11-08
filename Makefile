CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS =

SRC_DIR = src
BUILD_DIR = build
TARGET = distro-dep-name

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all clean install

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Dependencies
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(SRC_DIR)/parser.h $(SRC_DIR)/vm_query.h $(SRC_DIR)/distro.h $(SRC_DIR)/output.h
$(BUILD_DIR)/parser.o: $(SRC_DIR)/parser.c $(SRC_DIR)/parser.h
$(BUILD_DIR)/vm_query.o: $(SRC_DIR)/vm_query.c $(SRC_DIR)/vm_query.h $(SRC_DIR)/distro.h
$(BUILD_DIR)/distro.o: $(SRC_DIR)/distro.c $(SRC_DIR)/distro.h
$(BUILD_DIR)/output.o: $(SRC_DIR)/output.c $(SRC_DIR)/output.h $(SRC_DIR)/distro.h
