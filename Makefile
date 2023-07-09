CC = gcc

BIN = pihttpd

BIN_DIR = bin
OBJS_DIR = obj
SRC_DIR = src
CGI_DIR = $(SRC_DIR)/cgi

OBJS = ../$(OJBS_DIR)/%.o

CGI_SOURCES := $(wildcard ./$(CGI_DIR)/*.c)

CGI_FILES := $(patsubst ./$(CGI_DIR)/%.c, ./cgi/%.cgi ,$(CGI_SOURCES))

all:CHECK_DIR $(CGI_FILES) BUILD

# BUILD

cgi/%.cgi: $(CGI_DIR)/%.c
	@echo "CGI BUILD"
	$(CC) ./$(SRC_DIR)/p_*.c $< -o $@
	ls cgi
	@echo "CGI DOWN"

# 目前只能这样写，有点不会
BUILD: 
	$(CC) ./$(SRC_DIR)/*.c -o ./$(BIN_DIR)/$(BIN)
	@echo "RUNNING"
	@./$(BIN_DIR)/$(BIN)

CHECK_DIR: ECHO
	rm -rf $(BIN_DIR)
	rm -rf $(OBJS_DIR)
	rm -rf cgi
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJS_DIR)
	@mkdir -p cgi

ECHO:
	@echo "Start Build"
	pwd

clean:
	rm -rf $(BIN_DIR)
