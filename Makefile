NAME = webserv

UNAME_S := $(shell uname)

OBJ = $(SRC:.cpp=.o)

CPP = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98
IFLAGS = -I ./include/

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
	@ulimit -S -n 65536

$(NAME): $(OBJS)
	$(CPP) $(CFLAGS) -o $@ $(OBJS)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CPP) $(CFLAGS) $(IFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

debug : CFLAGS += -g3 -fsanitize=address
debug : fclean $(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)
	rm -rf logs uploads

re: fclean all

.PHONY: all clean fclean re debug
