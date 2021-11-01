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
#include "../stream.h" /* MAX_BLOCK_SIZE, readn(), writen() */
#include "token.h"

#define SERV_TCP_PORT 8080

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
        fgets(buf, sizeof(buf), stdin);
        nr = strlen(buf);
        if (buf[nr - 1] == '\n')
        {
            buf[nr - 1] = '\0';
            --nr;
        }
        bzero(buf2, sizeof(buf2));
        bcopy(buf, buf2, sizeof(buf));

        tknum = tokenise(buf2, tokens);

        if (strcmp(buf, "quit") == 0)
        {
            printf("Bye from client\n");
            exit(0);
        }
        if (nr > 0)
        {
            if ((nw = writen(sd, buf, nr)) < nr)
            {
                printf("client: send error\n");
                exit(1);
            }
            if ((nr = readn(sd, buf, sizeof(buf))) <= 0)
            {
                printf("client: receive error\n");
                exit(1);
            }
            buf[nr] = '\0';
            printf("Sever Output[%d]: %s\n", i, buf);
        }
    }
}