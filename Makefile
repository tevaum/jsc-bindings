LIB_CFLAGS=`pkg-config --cflags glib-2.0 javascriptcoregtk-3.0`
PRG_CFLAGS=$(LIB_CFLAGS)
LIBS=`pkg-config --libs javascriptcoregtk-3.0 glib-2.0`

all: clean main

main: javascript.o main.o
	$(CC) -g3 $(LIBS) main.o javascript.o -o main

javascript.o: javascriptcore/javascript.c
	$(CC) -g3 $(LIB_CFLAGS) -c javascriptcore/javascript.c -o javascript.o

main.o: main.c
	$(CC) -g3 $(PRG_CFLAGS) -c main.c -o main.o

clean:
	rm -f main *.o
