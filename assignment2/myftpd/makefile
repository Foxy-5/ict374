#Makefile

myftpd: myftpd.c token.o stream.o ../netprotocol.h
	gcc myftpd.c token.o stream.o ../netprotocol.h -o myftpd

token.o: ../token.c ../token.h
	gcc -c ../token.c -o token.o
	
stream.o: ../stream.c ../stream.h
	gcc -c ../stream.c -o stream.o
	
clean:
	rm *.o
