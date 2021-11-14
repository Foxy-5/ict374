/**
 * file:        myftp.c
 * Author:      Seow Wei Cheng (33753618) and Jin Min Seok ()
 * Date:        13/11/2021 (version 2)
 * Purpose:     This is the main driver code for the ftp client
 *              usage: myftp [ hostname | IP_address ]
 *              if no hostname or ip address is provided localhost is assumed
 *              default port is 41314
 *              The program can perform the following commands
 *              pwd - to display the current directory of the server that is serving the client;
 *              lpwd - to display the current directory of the client;
 *              dir - to display the file names under the current directory of the server that is serving the client;
 *              ldir - to display the file names under the current directory of the client;
 *              cd directory_pathname - to change the current directory of the server that is serving the client; Must support "." and ".." notations.
 *              lcd directory_pathname - to change the current directory of the client; Must support "." and ".." notations.
 *              get filename - to download the named file from the current directory of the remote server and save it in the current directory of the client;
 *              put filename - to upload the named file from the current directory of the client to the current directory of the remove server.
 *              quit - to terminate the myftp session.
 */
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* struct sockaddr_in, htons, htonl */
#include <netdb.h>      /* struct hostent, gethostbyname() */
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "../stream.h" /* MAX_BLOCK_SIZE, readn(), writen() */
#include "token.h"
#include "../netprotocol.h"

#define SERV_TCP_PORT 41314
//change client current directory
void cli_lcd(char *);
//list file in client current directory
void cli_ldir();
//get server change directory
void cli_pwd(int);
//list file in remote / server current directory
void cli_dir(int);
//Upload file from client to server
void cli_put(int, char *);
//download file from server to client
void cli_get(int, char *);
int main(int argc, char *argv[])
{
    int sd, n, nr, nw, tknum, i = 0;
    char buf[MAX_BLOCK_SIZE], buf2[MAX_BLOCK_SIZE], host[60];
    char *tokens[MAX_NUM_TOKENS];
    unsigned short port;
    struct sockaddr_in ser_addr;
    struct hostent *hp;
    /* get server host name and port number */
    if (argc == 1)
    { /* assume server running on the local host and on default port */
        strcpy(host, "localhost");
        port = SERV_TCP_PORT;
    }
    else if (argc == 2)
    { /* use the given host name */
        strcpy(host, argv[1]);
        port = SERV_TCP_PORT;
    }
    else
    {
        printf("Usage: %s [ <server host name> [ <server listening port> ] ]\n", argv[0]);
        exit(1);
    }

    /* get host address, & build a server socket address */
    bzero((char *)&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(port);
    if ((hp = gethostbyname(host)) == NULL)
    {
        printf("host %s not found\n", host);
        exit(1);
    }
    ser_addr.sin_addr.s_addr = *(u_long *)hp->h_addr;

    /* create TCP socket & connect socket to server address */
    sd = socket(PF_INET, SOCK_STREAM, 0);

    if (connect(sd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
    {
        perror("client connect");
        exit(1);
    }
    printf("Client has successfully connected to the server.\n");
    while (++i)
    {
        printf(">");
        //get user input
        fgets(buf, sizeof(buf), stdin);
        nr = strlen(buf);
        if (buf[nr - 1] == '\n')
        {
            buf[nr - 1] = '\0';
            --nr;
        }
        //quit
        if (strcmp(buf, "quit") == 0)
        {
            printf("\tBye from client\n");
            exit(0);
        }
        else
        {
            //set buffer to 0 and copy
            memset(buf2, 0, MAX_BLOCK_SIZE);
            memcpy(buf2, buf, MAX_BLOCK_SIZE);
            //tokenise user input
            tknum = tokenise(buf2, tokens);
            if (tknum > 2)
            {
                printf("\tInvalid command,please try again\n");
            }
            /*
            This will display client's current working directory
            */
            if (strcmp(tokens[0], "lpwd") == 0)
            {
                memset(buf, 0, MAX_BLOCK_SIZE);
                getcwd(buf, MAX_BLOCK_SIZE);
                printf("\t%s\n", buf);
            }
            else if (strcmp(tokens[0], "ldir") == 0)
            {
                cli_ldir();
            }
            /*
                This will change the client directory
                check the token for lcd, if = 0
                Continue to check if directory exist and then change to 
                that directory
                */
            else if (strcmp(tokens[0], "lcd") == 0)
            {
                if (tknum != 2)
                {
                    printf("\tInvalid command usage, please use: lcd [path]\n");
                }
                else
                {
                    cli_lcd(tokens[1]);
                }
            }
            else if (strcmp(tokens[0], "pwd") == 0)
            {
                cli_pwd(sd);
            }
            else if (strcmp(tokens[0], "dir") == 0)
            {
                cli_dir(sd);
            }
            else if (strcmp(tokens[0], "put") == 0)
            {
                if (tknum != 2)
                {
                    printf("\tInvalid command usage, please use: put [filename]\n");
                }
                else
                {
                    cli_put(sd, tokens[1]);
                }
            }
            else if (strcmp(tokens[0], "get") == 0)
            {
                if (tknum != 2)
                {
                    printf("\tInvalid command usage, please use: get [filename]\n");
                }
                else
                {
                    cli_get(sd, tokens[1]);
                }
            }
            else
            {
                printf("\tInvalid command please try again.\n");
            }
        }
    }
}

void cli_lcd(char *path)
{
    if (chdir(path) != 0)
    {
        printf("\tInvalid directory specified\n");
    }
    return;
}

void cli_ldir()
{
    //open and create dirent pointer and struct
    DIR *dp;
    struct dirent *direntp;
    //variables for file name storage
    int filecount = 0;
    char *filenamearray[MAX_NUM_TOKENS];
    if ((dp = opendir(".")) == NULL)
    {
        printf("\tFailed to open directory\n");
        return;
    }
    //get filenames
    while ((direntp = readdir(dp)) != NULL)
    {
        if (direntp->d_name[0] == '.')
        {
            continue;
        }
        filenamearray[filecount] = direntp->d_name;
        filecount++;
        if (filecount > MAX_NUM_TOKENS)
        {
            printf("\tToo many files to be displayed!\n");
            break;
        }
    }
    for (int i = 0; i < filecount; i++)
    {
        printf("\t%s\n", filenamearray[i]);
    }
    return;
}

void cli_pwd(int sd)
{
    //create variable for use
    char buf[MAX_BLOCK_SIZE];
    char serverpath[MAX_BLOCK_SIZE];
    int nw, nr, len;
    char opcode;
    memset(buf, 0, MAX_BLOCK_SIZE);
    memset(serverpath, 0, MAX_BLOCK_SIZE);
    //set op code
    buf[0] = PWD_CODE;
    //write op code to server
    nw = writen(sd, buf, 1);
    //read the op code from the server
    nr = readn(sd, &buf[0], sizeof(buf));
    //printf("\t%s\n",buf);
    if (!(buf[0] == PWD_CODE))
    {
        printf("/tFailed to read PWD op code\n");
        return;
    }
    nr = readn(sd, &buf[1], sizeof(buf));
    if (buf[1] == PWD_READY)
    {
        // read the length of server file path
        nr = readn(sd, &buf[2], sizeof(buf));
        //copy 4 bytes of the length
        memcpy(&len, &buf[2], 4);
        len = (int)ntohl(len);
        nr = readn(sd, serverpath, sizeof(buf));
        printf("\t%s\n", serverpath);
    }
    else
    {
        printf("\tFailed: Status code was '1'\n");
    }
    return;
}

void cli_dir(int sd)
{
    //create variable for use
    char buf[MAX_BLOCK_SIZE];
    char ser_files[MAX_BLOCK_SIZE];
    int nw, nr, len;
    char opcode;
    memset(buf, 0, MAX_BLOCK_SIZE);
    memset(ser_files, 0, MAX_BLOCK_SIZE);
    //set op code
    buf[0] = DIR_CODE;
    //write op code to server
    nw = writen(sd, buf, 1);
    //read the op code from the server
    nr = readn(sd, &buf[0], sizeof(buf));
    //printf("\t%s\n",buf);
    if (!(buf[0] == DIR_CODE))
    {
        printf("\tFailed to read DIR op code\n");
        return;
    }
    nr = readn(sd, &buf[1], sizeof(buf));
    if (buf[1] == DIR_READY)
    {
        // read the length of server file path
        nr = readn(sd, &buf[2], sizeof(buf));
        //copy 4 bytes of the length
        memcpy(&len, &buf[2], 4);
        len = (int)ntohl(len);
        nr = readn(sd, ser_files, sizeof(buf));
        printf("\t%s\n", ser_files);
    }
    else
    {
        printf("\tFailed: Status code was '1'\n");
    }
    return;
}

void cli_get(int sd, char *filename)
{
    char opcode, ackcode;
    int fsize, nr, nw, file_len, total, fd;
    char buf[MAX_BLOCK_SIZE];
    memset(buf, 0, MAX_BLOCK_SIZE);

    char file_name[MAX_BLOCK_SIZE]; //use for storing filename
    strcpy(file_name, filename);    //string copy filename
    buf[0] = GET_1;
    if (writen(sd, &buf[0], 1) < 0)
    {
        printf("\tFailed to write op code to server.\n");
        return;
    }
    memset(buf, 0, MAX_BLOCK_SIZE); //set buffer to 0
    //get file name length
    file_len = strlen(file_name);
    int templen = htons(file_len);
    //set last index of file_name to null
    file_name[file_len] = '\0';
    //write file name length to server
    memcpy(&buf[0], &templen, 2);
    if (writen(sd, &buf[0], 2) < 0)
    {
        printf("\tFailed to write file length to server.\n");
        return;
    }
    //write file name to server using file length
    memcpy(&buf[2], &file_name, file_len);
    if (writen(sd, &buf[2], file_len) < 0)
    {
        printf("\tFailed to write file name to server.\n");
        return;
    }
    //read server response
    memset(buf, 0, MAX_BLOCK_SIZE);
    if (readn(sd, &buf[0], MAX_BLOCK_SIZE) < 0)
    {
        printf("\tFailed to read op code from server\n");
        return;
    }
    memcpy(&opcode, &buf[0], 1);
    printf("\tOpcode from server is: %c\n", opcode);
    if (readn(sd, &buf[1], MAX_BLOCK_SIZE) < 0)
    {
        printf("\tFailed to read ack code from server\n");
        return;
    }
    memcpy(&ackcode, &buf[1], 1);
    printf("\tAckcode from server is: %c\n", ackcode);
    if (opcode == GET_1)
    {
        if (ackcode == GET_READY)
        {
            printf("\tfile is found on server.\n");
            memset(buf, 0, MAX_BLOCK_SIZE);
            if (readn(sd, &buf[0], MAX_BLOCK_SIZE) < 0)
            {
                printf("\tFailed to read opcode 2 from server.\n");
                return;
            }
            memcpy(&opcode, &buf[0], 1);
            if (opcode == GET_2)
            {
                //check if can read file size from client
                if (readn(sd, &buf[1], MAX_BLOCK_SIZE) < 0)
                {
                    printf("\tfailed to read file size from client\n");
                    return;
                }
                memcpy(&fsize, &buf[1], 4);
                //convert file size to host byte order
                fsize = ntohl(fsize);
                printf("\tfile size is %d\n", fsize);
                //create file
                if ((fd = open(filename, O_WRONLY | O_CREAT, 0666)) != -1)
                {
                    //set ackcode
                    ackcode = PUT_DONE;
                    //set max block of data recieved
                    char block[MAX_BLOCK_SIZE];
                    //set block buffer to 0
                    memset(block, '\0', MAX_BLOCK_SIZE);
                    //set file seek pointer to start of file
                    lseek(fd, 0, SEEK_SET);
                    //read first block
                    nr = readn(sd, block, MAX_BLOCK_SIZE);
                    //if failed to read set ackcode to '1'
                    if (nr < 0)
                    {
                        printf("failed to read file\n");
                        ackcode = PUT_FAIL;
                    }
                    //if total file size is smaller than max block size
                    if (fsize < MAX_BLOCK_SIZE)
                    {
                        //write block of data to file using leftover file size
                        nw = write(fd, block, fsize);
                    }
                    else
                    {
                        //write block of data to file using max block size
                        nw = write(fd, block, MAX_BLOCK_SIZE);
                        //add write count to total count
                        total += nw;
                        //if total transfer is less than file size
                        while (total < fsize)
                        {
                            //reset block buffer
                            memset(block, '\0', MAX_BLOCK_SIZE);
                            //set file seek pointer to total transfer
                            lseek(fd, total, SEEK_SET);
                            //read next block of data
                            nr = readn(sd, block, MAX_BLOCK_SIZE);
                            //if leftover data is less than max block size
                            if ((fsize - total) < MAX_BLOCK_SIZE)
                            {
                                //write to fd using the leftover file size
                                nw = write(fd, block, (fsize - total));
                            }
                            else
                            {
                                //write to fd using the max block size
                                nw = write(fd, block, MAX_BLOCK_SIZE);
                            }
                            //add to total file count
                            total += nw;
                        }
                    }
                }
                printf("\tFile is recieved from server.\n");
            }
            else if (ackcode == GET_NOT_FOUND)
            {
                printf("\tError:file is not found on server.\n");
                return;
            }
        }
        else
        {
            printf("\tServer send wrong opcode\n");
            return;
        }
    }
}
void cli_put(int sd, char *filename)
{

    //variable used
    char opcode, ackcode; //storing opcode and ackcode
    int fsize, nr, nw, file_len, total;
    char buf[MAX_BLOCK_SIZE]; //to store message from client or server

    memset(buf, 0, MAX_BLOCK_SIZE); //set buffer to zero
    char file_name[MAX_BLOCK_SIZE]; //use for storing filename
    strcpy(file_name, filename);    //string copy filename

    FILE *file;                   //create file pointer
    file = fopen(file_name, "r"); //open client selected file
    //check if file exist on client
    if (file != NULL)
    {
        //write opcode to server
        buf[0] = PUT_1;
        writen(sd, &buf[0], 1);
        memset(buf, 0, MAX_BLOCK_SIZE); //set buffer to 0
        //get file name length
        file_len = strlen(file_name);
        int templen = htons(file_len);
        //set last index of file_name to null
        file_name[file_len] = '\0';
        //write file name length to server
        memcpy(&buf[0], &templen, 2);
        writen(sd, &buf[0], 2);
        //write file name to server using file length
        memcpy(&buf[2], &file_name, file_len);
        writen(sd, &buf[2], file_len);

        //read server response
        memset(buf, 0, MAX_BLOCK_SIZE);
        //check if server opcode response received
        if (readn(sd, &buf[0], MAX_BLOCK_SIZE) < 0)
        {
            printf("\tfailed to read opcode from server.\n");
            return;
        }
        else
        {
            memcpy(&opcode, &buf[0], 1);
            printf("\topcode from server is: %c\n", opcode);
        }
        //check if server ackcode response received
        if (readn(sd, &buf[1], MAX_BLOCK_SIZE) < 0)
        {
            printf("\tfailed to read ackcode from server.\n");
            return;
        }
        else
        {
            memcpy(&ackcode, &buf[1], 1);
            printf("\topcode from server is: %c\n", ackcode);
        }
        //if ackcode is 0
        if (ackcode == PUT_READY)
        {
            //write opcode 2 to server
            memset(buf, 0, MAX_BLOCK_SIZE);
            opcode = PUT_2;
            memcpy(&buf[0], &opcode, 1);
            if (writen(sd, &buf[0], 1) < 0)
            {
                printf("\tfailed to write opcode 2 to server\n");
                return;
            }
            //get file size and send to server
            struct stat fst;
            //check if file stat is ok
            if (stat(file_name, &fst) == -1)
            {
                printf("\t failed to get file stat\n");
                return;
            }
            //get file size and convert it to network btye order
            fsize = (int)fst.st_size;
            templen = htonl(fsize);
            memcpy(&buf[1], &templen, 4);
            //check if file size is send to server
            if (writen(sd, &buf[1], 4) < 0)
            {
                printf("\tfailed to write file size to server\n");
                return;
            }
            //getting file descriptor
            int fd = fileno(file);
            //creating buffer for block of data
            char block[MAX_BLOCK_SIZE];
            memset(block, '\0', MAX_BLOCK_SIZE);
            //set file pointer
            lseek(fd, 0, SEEK_SET);
            //read and write first block of data
            nr = read(fd, block, MAX_BLOCK_SIZE);
            nw = writen(sd, block, MAX_BLOCK_SIZE);
            //total write data
            total += nw;
            //if current sent block of data is smaller than total file size
            while (total < fsize)
            {
                //reset block buffer
                memset(block, '\0', MAX_BLOCK_SIZE);
                //set file seek pointer to previous end
                lseek(fd, total, SEEK_SET);

                //read next block of data
                //if file size - current total size is larger than max block
                if ((fsize - total) > MAX_BLOCK_SIZE)
                {
                    //read next block of data to max block size
                    nr = read(fd, block, MAX_BLOCK_SIZE);
                }
                else
                {
                    //read next block of data to leftover size
                    nr = read(fd, block, (fsize - total));
                }
                //read block data to server
                writen(sd, block, MAX_BLOCK_SIZE);
                //add write count to total size
                total += nr;
            }

            //read server response
            memset(buf, 0, MAX_BLOCK_SIZE);
            //check if can read opcode from server
            if (readn(sd, &buf[0], MAX_BLOCK_SIZE) < 0)
            {
                printf("\tError: Not able to read opcode from server 3.\n");
                return;
            }
            memcpy(&opcode, &buf[0], 1);
            //check if can read ack code from server
            if (readn(sd, &buf[1], MAX_BLOCK_SIZE))
            {
                printf("\tError: Not able to read ack code from server 3.\n");
            }
            memcpy(&ackcode, &buf[1], 1);
            //check if opcode is correct
            if (opcode == PUT_2)
            {
                printf("\tServer opcode recieved.\n");
                printf("\topcode is: %c\n", opcode);
                //check if ackcode is 0, which is file transfer done
                if (ackcode == PUT_DONE)
                {
                    printf("\tFile is transfer succesfully.\n");
                    printf("\tackcode is: %c\n", ackcode);
                }
                else if (ackcode == PUT_FAIL)
                {
                    printf("\tFile failed to transfer succesfully.\n");
                    printf("\tackcode is: %c\n", ackcode);
                }
            }
        }
        //if ackcode returns to be '1'
        else if (ackcode == PUT_CLASH_ERROR)
        {
            printf("\tFile already exist on server\n");
            return;
        }
        // else if (ackcode == PUT_CREATE_ERROR)
        // {
        //     printf("\tServer failed to crate file\n");
        //     return;
        // }
    }
    //cannot open file
    else
    {
        printf("\tFile cannot be open.\n");
        return;
    }
    return;
}