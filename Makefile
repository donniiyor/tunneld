CC := clang

SERVER_TARGET := tunnels
CLIENT_TARGET := tunnelc

BUILD_DIR := build

COMMON_DIR := common
SERVER_DIR := server
CLIENT_DIR := client

CFLAGS := -Wall -Wextra -Wpedantic -std=c11
CFLAGS += -Icommon
CFLAGS += -Iserver
CFLAGS += -Iclient

LDFLAGS :=

COMMON_SRCS := $(wildcard $(COMMON_DIR)/*.c)
SERVER_SRCS := $(COMMON_SRCS) $(wildcard $(SERVER_DIR)/*.c)
CLIENT_SRCS := $(COMMON_SRCS) $(wildcard $(CLIENT_DIR)/*.c)

SERVER_OBJS := $(patsubst %.c,$(BUILD_DIR)/server/%.o,$(SERVER_SRCS))
CLIENT_OBJS := $(patsubst %.c,$(BUILD_DIR)/client/%.o,$(CLIENT_SRCS))

.PHONY: all clean run-server run-client

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(SERVER_OBJS) -o $@ $(LDFLAGS)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CLIENT_OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/server/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/client/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run-server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

run-client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET)

clean:
	rm -rf $(BUILD_DIR) $(SERVER_TARGET) $(CLIENT_TARGET)
