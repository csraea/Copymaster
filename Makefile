CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra
LDFLAGS =
SOURCES = copymaster.c options.c
HEADERS = options.h
OBJDIR = obj
EXECUTABLE = copymaster

_OBJ = $(SOURCES:.c=.o)
_DEPS = $(HEADERS)

# DEPS = $(patsubst %,./%,$(_DEPS))
DEPS = $(_DEPS)
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))

$(OBJDIR)/%.o: %.c $(DEPS)
	mkdir -p $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXECUTABLE): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

all: $(EXECUTABLE)

clean:
	rm -f $(OBJDIR)/*.o $(EXECUTABLE)
	if [ -d $(OBJDIR) ]; then rmdir $(OBJDIR); fi

.PHONY: all clean
