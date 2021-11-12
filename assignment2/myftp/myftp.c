/**
 * file:        myftp.c
 * Author:      Seow Wei Cheng (33753618)
 * Date:        28/10/2021 (version 1)
 * Purpose:     This is the main driver code for the ftp client
 *              usage: myftp [ hostname | IP_address ]
 *              if no hostname or ip address is provided localhost is assumed
 *              default port is 8080
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

#define SERV_TCP_PORT 8080
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

void cli_put(int sd, char *filename)
{
    char opcode, ackcode;
    int fsize, nr, nw, file_len, total;
    char buf[MAX_BLOCK_SIZE];
    memset(buf, 0, MAX_BLOCK_SIZE);
    char file_name[MAX_BLOCK_SIZE];
    strcpy(file_name, filename);

    FILE *file;
    file = fopen(file_name, "r");
    if (file != NULL)
    {
        //write opcode to server
        buf[0] = PUT_1;
        writen(sd, &buf[0], 1);
        memset(buf, 0, MAX_BLOCK_SIZE);
        //get file name length
        file_len = strlen(file_name);
        int templen = htons(file_len);
        file_name[file_len] = '\0';
        //write file name length to server
        memcpy(&buf[0], &templen, 2);
        writen(sd, &buf[0], 2);
        memcpy(&buf[2], &file_name, file_len);
        writen(sd, &buf[2], file_len);

        //read server response
        memset(buf, 0, MAX_BLOCK_SIZE);
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
            if (stat(file_name, &fst) == -1)
            {
                printf("\t failed to get file stat\n");
            }
            fsize = (int)fst.st_size;
            templen = htonl(fsize);
            memcpy(&buf[1], &templen, 4);

            if (writen(sd, &buf[1], 4) < 0)
            {
                printf("\tfailed to write file size to server\n");
                return;
            }
            int fd = fileno(file);
            char block[MAX_BLOCK_SIZE];
            memset(block, '\0', MAX_BLOCK_SIZE);
            //set file pointer
            lseek(fd, 0, SEEK_SET);
            //read and write first block of data
            nr = read(fd, block, MAX_BLOCK_SIZE);
            nw = writen(sd, block, MAX_BLOCK_SIZE);
            //total write data
            total += nw;

            while (total < fsize)
            {
                memset(block, '\0', MAX_BLOCK_SIZE);
                lseek(fd, total, SEEK_SET);

                //read next block of data
                if ((fsize - total) > MAX_BLOCK_SIZE)
                {
                    nr = read(fd, block, MAX_BLOCK_SIZE);
                }
                else
                {
                    nr = read(fd, block, (fsize - total));
                }
                writen(sd, block, MAX_BLOCK_SIZE);
                total += nr;
            }
            
            //read server response
            memset(buf, 0, MAX_BLOCK_SIZE);
            readn(sd,&buf[0],MAX_BLOCK_SIZE);
            memcpy(&opcode,&buf[0],1);
            readn(sd,&buf[1],MAX_BLOCK_SIZE);
            memcpy(&ackcode,&buf[1],1);
            if(opcode == PUT_2)
            {
                printf("\tServer opcode recieved.\n");
                printf("\topcode is: %c\n", opcode);
                if(ackcode == PUT_DONE)
                {
                    printf("\tFile is transfer succesfully.\n");
                    printf("\tackcode is: %c\n", ackcode);
                }
                else if(ackcode == PUT_FAIL)
                {
                    printf("\tFile failed to transfer succesfully.\n");
                    printf("\tackcode is: %c\n", ackcode);
                }
            }
        }
        else if (ackcode == PUT_CLASH_ERROR)
        {
            printf("\tFile already exist on server\n");
            return;
        }
        else if (ackcode == PUT_CREATE_ERROR)
        {
            printf("\tServer failed to crate file\n");
            return;
        }
    }
    else
    {
        printf("\tFile cannot be open.\n");
        return;
    }
    return;
}
// void cli_put(int sd, char *filename)
// {
//     //variables
//     char opcode, ackcode;
//     int fd, fsize, nr, nw, convertedsize, convertedfile_len, block_size;
//     struct stat filestat;
//     char buf[MAX_BLOCK_SIZE];
//     char buf1[MAX_BLOCK_SIZE];

//     int file_len = strlen(filename); //file name length
//     char file_name[file_len + 1];    //file name size
//     strcpy(file_name, filename);
//     filename[file_len] = '\0';
//     //check if file exist
//     if ((fd = open(file_name, O_RDONLY) == -1))
//     {
//         printf("\tFailed to open selected file.\n");
//         return;
//     }

//     //check file stats
//     if (fstat(fd, &filestat) < 0)
//     {
//         printf("\tFailed to get file stats.\n");
//         return;
//     }
//     fsize = (int)filestat.st_size;
//     convertedsize = (int)htonl(fsize);
//     //send opcode to server
//     opcode = PUT_1;
//     buf1[0] = opcode;
//     if (writen(sd, buf1, 1) < 0)
//     {
//         printf("\tFailed to send opcode to server.\n");
//         return;
//     }

//     //send file name length to server
//     convertedfile_len = htons(file_len);
//     //memcpy(&buf1[1],&convertedfile_len,4);
//     bcopy(&convertedfile_len,&buf1[1],2);
//     if (writen(sd, &buf1[1], 2) < 0)
//     {
//         printf("\tfailed to send file name length to server.\n");
//         return;
//     }

//     //send file name
//     if (writen(sd, file_name, file_len) < 0)
//     {
//         printf("\tFailed to send file name to server.\n");
//         return;
//     }

//     //read opcode from server
//     if (readn(sd, &opcode, MAX_BLOCK_SIZE) < 0)
//     {
//         printf("\tFailed to read opcode from server.\n");
//         return;
//     }
//     //check opcode from server
//     if (opcode != PUT_1)
//     {
//         printf("\tFailed to get correct opcode from server.\n");
//         return;
//     }
//     //read ackcode from server
//     if (readn(sd, &ackcode, MAX_BLOCK_SIZE) < 0)
//     {
//         printf("\tFailed to read ack code from server\n");
//         return;
//     }
//     //check ack code from server
//     if (ackcode == PUT_READY)
//     {
//         opcode = PUT_2;
//         if (writen(sd, &opcode, 1) < 0)
//         {
//             printf("\tFailed to write opcode 2 to server\n");
//             return;
//         }
//         memset(buf1,'\0',MAX_BLOCK_SIZE);
//         memcpy(buf1,&convertedsize,4);
//         if(writen(sd,buf1,4) < 0)
//         {
//             printf("\tFailed to write file size to server.\n");
//             return;
//         }
//         memset(buf1,'\0',MAX_BLOCK_SIZE);
//         lseek(fd,0,SEEK_SET);
//         nr = read(fd, buf1, MAX_BLOCK_SIZE);
//         nw = writen(sd, buf1, MAX_BLOCK_SIZE);

//         block_size += nw;

//         while(block_size < fsize)
//         {
//             memset(buf1,'\0',MAX_BLOCK_SIZE);
//             lseek(fd,block_size,SEEK_SET);

//             if((fsize - block_size) > MAX_BLOCK_SIZE)
//             {
//                 nr = read(fd, buf1, MAX_BLOCK_SIZE);
//             }
//             else
//             {
//                 nr = read(fd,buf1,(fsize - block_size));
//             }
//             writen(sd, buf1, MAX_BLOCK_SIZE);
//             block_size += nr;
//         }
//     }
//     else if (ackcode == PUT_CLASH_ERROR)
//     {
//         printf("\tServer return error code 1\n");
//         return;
//     }
//     else if (ackcode == PUT_CREATE_ERROR)
//     {
//         printf("\tServer return error code 2\n");
//         return;
//     }
//     else if (ackcode == PUT_OTHER_ERROR)
//     {
//         printf("\tServer return error code 3\n");
//         return;
//     }
//     close(fd);
//     return;
// }