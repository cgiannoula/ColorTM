CC = gcc
CFLAGS = -O3
LDFLAGS = -pthread -lm  #-lnuma

all: ColorTM 

## 
ColorTM: test_graphColor.o ColorTM.o graph.o transaction.o
	$(CC) $(LDFLAGS)  -mrtm -o ColorTM test_graphColor.o ColorTM.o graph.o transaction.o

test_graphColor.o: test_graphColor.c
	$(CC) $(CFLAGS)  -mrtm -c -o test_graphColor.o test_graphColor.c

ColorTM.o: graphColor.c 
	$(CC) $(CFLAGS)  -mrtm -c -o ColorTM.o graphColor.c

graph.o: graph.c graph.h 
	$(CC) $(CFLAGS) -c -o graph.o graph.c

transaction.o: transaction.c transaction.h
	$(CC) $(CFLAGS) -mrtm -c -o transaction.o transaction.c 

clean:
	rm -f *.o ColorTM 
