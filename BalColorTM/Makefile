CC = gcc
CFLAGS = -O3
LDFLAGS = -lm #-lnuma

all: BalColorTM

##
BalColorTM: test_graphColor.o BalColorTM.o graph.o transaction.o
	$(CC) $(LDFLAGS)  -fopenmp -o BalColorTM test_graphColor.o BalColorTM.o graph.o transaction.o 

test_graphColor.o: test_graphColor.c
	$(CC) $(CFLAGS) -fopenmp  -mrtm -c -o test_graphColor.o test_graphColor.c

BalColorTM.o: graphColor.c 
	$(CC) $(CFLAGS) -fopenmp  -mrtm -c -o BalColorTM.o graphColor.c

graph.o: graph.c graph.h 
	$(CC) $(CFLAGS) -c -o graph.o graph.c

transaction.o: transaction.c transaction.h
	$(CC) $(CFLAGS) -mrtm -c -o transaction.o transaction.c 

clean:
	rm -f *.o BalColorTM 
