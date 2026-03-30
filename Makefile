CC = gcc
CFLAGS = -Wall -Wextra -lm
SRC = src/sim.c
OUT = sim.o

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
