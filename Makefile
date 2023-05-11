SRCS			= Server.cpp Channel.cpp main.cpp Client.cpp 
OBJS			= $(SRCS:.cpp=.o)
HEADERS			= Server.hpp Client.hpp Channel.hpp

CXXFLAGS		= -Wall -Wextra -Werror -std=c++98 

NAME			= ircserv

all:			$(NAME)

$(NAME):		$(OBJS) $(HEADERS)
				c++ $(CXXFLAGS) -o $(NAME) $(OBJS) 

clean:
				rm -f $(OBJS)

fclean:			clean
				rm -f $(NAME)

re:				fclean $(NAME)