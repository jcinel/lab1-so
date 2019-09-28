CC=gcc
CFLAGS=-Wall -Wunreachable-code

SDIR=src
ODIR=obj

_OBJ=main.o shell.o parser.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

shell: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)
