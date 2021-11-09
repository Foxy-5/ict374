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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> /* struct sockaddr_in, htons, htonl */
#include <netdb.h>      /* struct hostent, gethostbyname() */
#include <string.h>
#include <unistd.h>
#include <dirent.h>
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
        len = (int)ntohs(len);
        nr = readn(sd, serverpath, sizeof(buf));
        printf("\t%s\n", serverpath);
    }
    else
    {
        printf("\tFailed: Status code was '1'\n");
    }
    return;
}