CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lm
SRC = src/sim.c
OUT = sim.o

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDLIBS)

clean:
	rm -f $(OUT)
