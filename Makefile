CC=g++
CSOURCES=microcat.c test_parser.c shell.c
CEXES := $(subst .c,,$(CSOURCES))
FLAGS=-g -Wno-unused-variable -Wno-unused-but-set-variable
LIB_DIR=-L.
INCLUDE_DIR=-I.

# By default, make runs the first target in the file
all: $(CEXES) libparser.so

% :: %.c libparser.so
	$(CC) $(FLAGS) $(INCLUDE_DIR) $(LIB_DIR) -Wl,-rpath=. $< -o $@ -lreadline -lparser

libparser.so : libparser.c
	$(CC) $(FLAGS) $(INCLUDES) -fPIC -c $< -o libparser.o
	$(CC) -shared -o $@ libparser.o


clean:
	rm -rf $(CEXES) libparser.so

