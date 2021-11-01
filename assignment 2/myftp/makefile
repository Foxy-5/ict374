#Makefile

myftp: myftp.c token.o stream.o
	gcc myftp.c token.o stream.o -o myftp
	
token.o: token.c token.h
	gcc -c token.c -o token.o
	
stream.o: ../stream.c ../stream.h
	gcc -c ../stream.c -o stream.o
	
clean:
	rm *.o
