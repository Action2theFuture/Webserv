COLOR_RESET = \033[0m
COLOR_RED = \033[1;31m
COLOR_GREEN = \033[1;32m
COLOR_YELLOW = \033[1;33m
COLOR_BLUE = \033[1;34m
COLOR_CYAN = \033[1;36m

NAME = webserv
UNAME_S := $(shell uname)

SPINNER_SCRIPT = assets/spinner.sh

OBJ = $(SRC:.cpp=.o)

CPP = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98
IFLAGS = -I ./include/

LOG_DIR = logs
SRC_DIR = src
OBJ_DIR = obj
SERVER_DIR = $(SRC_DIR)/Server
PARSING_DIR = $(SRC_DIR)/Parsing
REQUEST_DIR = $(SRC_DIR)/Request
RESPONSE_DIR = $(SRC_DIR)/Response
POLLER_DIR = $(SRC_DIR)/Poller

SRC = main.cpp Utils.cpp Log.cpp
PARSING = ConfigurationCore.cpp ConfigurationParse.cpp HttpMultipartParser.cpp \
	HttpParserUtils.cpp HttpRequestParser.cpp
SERVER = ServerCore.cpp ServerMatchLocation.cpp SocketManager.cpp \
	ServerUtils.cpp ServerWrite.cpp ServerEvents.cpp ServerWriteHelper.cpp
REQUEST = Request.cpp
RESPONSE = Response.cpp ResponseHandlers.cpp ResponseUtils.cpp \
		CGIHandler.cpp

SRCS := $(addprefix $(SRC_DIR)/, $(SRC))
SRCS += $(addprefix $(PARSING_DIR)/, $(PARSING))
SRCS += $(addprefix $(SERVER_DIR)/, $(SERVER))
SRCS += $(addprefix $(REQUEST_DIR)/, $(REQUEST))
SRCS += $(addprefix $(RESPONSE_DIR)/, $(RESPONSE))

vpath %.cpp ./src/

ifeq ($(UNAME_S), Linux)
    CFLAGS += -D__linux__
	SRCS += $(addprefix $(POLLER_DIR)/, EpollPoller.cpp)
else ifeq ($(UNAME_S), Darwin)
    CFLAGS += -D__APPLE__
	SRCS += $(addprefix $(POLLER_DIR)/, KqueuePoller.cpp)
endif

OBJS := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS))


all: $(NAME) 

$(NAME): $(OBJS)
	@$(CPP) $(CFLAGS) -o $@ $(OBJS) > /dev/null 2>&1 & COMPILER_PID=$$!; \
	./$(SPINNER_SCRIPT) $$COMPILER_PID; \
	wait $$COMPILER_PID
	@echo "$(COLOR_GREEN)Program Name : $(NAME)$(COLOR_RESET)"

OBJ_FILES_SPINNER_PID=

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@$(CPP) $(CFLAGS) $(IFLAGS) -c $< -o $@ > /dev/null 2>&1

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

dev : CFLAGS += -DDEV_MODE
dev : fclean $(NAME)
	@echo "$(COLOR_GREEN)Start DeVMoDe! 🛠️$(COLOR_RESET)"

debug : CFLAGS += -g3 -fsanitize=address
debug : fclean $(NAME)
	@echo "$(COLOR_GREEN)Start Debugging! 🛠️$(COLOR_RESET)"

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(COLOR_RED)Cleaning completed successfully 🧹$(COLOR_RESET)"

fclean: clean
	@rm -f $(NAME)
	@rm -rf logs uploads
	@echo "$(COLOR_RED)Full Cleaning completed successfully 🧹$(COLOR_RESET)"

re: fclean all

.PHONY: all dev clean fclean re debug
