CC := clang

TARGET := tunneld

BUILD_DIR := build

COMMON_DIR := common
SERVER_DIR := server
CLIENT_DIR := client
INC_DIR := include

CFLAGS := -Wall -Wextra -Wpedantic -std=c11
CFLAGS += -Icommon
CFLAGS += -Iserver
CFLAGS += -Iclient

LDFLAGS :=

SRCS := \
	$(wildcard $(COMMON_DIR)/*.c) \
	$(wildcard $(SERVER_DIR)/*.c) \
	$(wildcard $(CLIENT_DIR)/*.c)

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
