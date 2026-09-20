NAME		:= codexion

CC			:= cc
CFLAGS		:= -Wall -Wextra -Werror -pthread
CPPFLAGS	:= -Iincludes -MMD -MP

SRC_DIR		:= src
OBJ_DIR		:= obj

SRCS		:= main.c parse_args.c time.c log.c heap.c heap_utils.c \
			   dongle.c acquire.c event.c coder.c monitor.c init.c threads.c cleanup.c
OBJS		:= $(addprefix $(OBJ_DIR)/, $(SRCS:.c=.o))
DEPS		:= $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
