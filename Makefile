NAME = webserv

UNAME_S := $(shell uname)

OBJ = $(SRC:.cpp=.o)

CPP = c++
CFLAGS = -Wall -Wextra -Werror -std=c++98
IFLAGS = -I ./include/

SRC_DIR = src
OBJ_DIR = obj

SRC = main.cpp Server.cpp Request.cpp Response.cpp Utils.cpp \
	CGIHandler.cpp Configuration.cpp SocketManager.cpp RequestHandler.cpp

SRCS := $(addprefix $(SRC_DIR)/, $(SRC))

vpath %.cpp ./src/

ifeq ($(UNAME_S), Linux)
    CFLAGS += -D__linux__
	SRCS += $(addprefix $(SRC_DIR)/, EpollPoller.cpp)
else ifeq ($(UNAME_S), Darwin)
    CFLAGS += -D__APPLE__
	SRCS += $(addprefix $(SRC_DIR)/, KqueuePoller.cpp)
endif

OBJS := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(NAME)

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

re: fclean all

.PHONY: all clean fclean re debug
