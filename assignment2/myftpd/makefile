#Makefile

myftp: myftpd.c stream.o ../netprotocol.h
	gcc myftpd.c stream.o ../netprotocol.h -o myftpd
	
stream.o: ../stream.c ../stream.h
	gcc -c ../stream.c -o stream.o
	
clean:
	rm *.o
