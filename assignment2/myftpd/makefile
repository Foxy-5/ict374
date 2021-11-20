#Makefile

myftpd: myftpd.c token.o stream.o ../netprotocol.h
	gcc -Wall myftpd.c token.o stream.o ../netprotocol.h -o myftpd

token.o: ../token.c ../token.h
	gcc -Wall -c ../token.c -o token.o
	
stream.o: ../stream.c ../stream.h
	gcc -Wall -c ../stream.c -o stream.o
	
clean:
	rm *.o
