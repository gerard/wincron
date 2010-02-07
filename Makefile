# MinGW for Linux (cross compilation)
CC=i586-mingw32msvc-gcc
RM=rm -f

## MinGW for Windows
#CC=gcc
#RM=del

all: cron.exe recron.exe

cron.exe: cron.o lexico.o sintactico.o
	$(CC) -o cron.exe cron.o lexico.o sintactico.o

recron.exe: recron.c
	$(CC) -o recron.exe recron.c

lexico.o: lexico.c lexico.h
	$(CC) -c lexico.c

sintactico.o: sintactico.c sintactico.h lexico.h
	$(CC) -c sintactico.c

cron.o: cron.c sintactico.h
	$(CC) -c cron.c

clean:
	$(RM) recron.exe cron.exe *.o 
