#Makefile

myftp: myftpd.c stream.o
	gcc myftpd.c stream.o -o myftpd
	
stream.o: ../stream.c ../stream.h
	gcc -c ../stream.c -o stream.o
	
clean:
	rm *.o
