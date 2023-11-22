CC=cc
CFLAGS=-Wall -g -pedantic -Werror -Wextra -std=c99 
LIBS=-lcurl 
BIN=quran-cli
SRC=main.c

all: $(SRC) 
	$(CC) $(SRC) -o $(BIN) $(LIBS) $(CFLAGS) -fsanitize=address
