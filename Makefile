SRCS			= sources/Server.cpp sources/Channel.cpp sources/main.cpp sources/Client.cpp sources/parsing.cpp sources/commands.cpp
OBJS			= $(SRCS:.cpp=.o)
HEADERS			= includes/Server.hpp includes/Client.hpp includes/Channel.hpp

CXXFLAGS		= -Wall -Wextra -Werror -std=c++98

NAME			= ircserv

all:			$(NAME)

$(NAME):		$(OBJS) $(HEADERS)
				c++ $(CXXFLAGS) -o $(NAME) $(OBJS) -fsanitize=address

clean:
				rm -f $(OBJS)

fclean:			clean
				rm -f $(NAME)

re:				fclean $(NAME)