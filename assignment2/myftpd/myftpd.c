/**
 * file:        myftpd.c
 * Author:      Seow Wei Cheng (33753618)
 * Date:        28/10/2021 (version 1)
 * Purpose:     This is the main driver code for the ftp server
 *              usage: myftpd [initial_current_directory]
 *              if no initial directory is provided current directory is assumed
 *              default port is 8080
 *              The program can perform the following commands
 *              - [pwd] Display the current directory of the server that is currently serving the client
 *              - [dir] Display the file names under the current directory that is serving the client
 *              - [cd] [directory_pathname] Change the current directory that is serving the client
 *              - [get] [filename] Transfer the file from the current directory of the server to the client 
 *              - [put] [filename] Transfer the file from the client to the current directory of the server
 *              - [quit] Terminate the session with the client
 */
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h> /* strlen(), strcmp() etc */
#include <stdio.h>  /* printf()  */
#include <string.h> /* strlen(), strcmp() etc */
#include <errno.h>  /* extern int errno, EINTR, perror() */
#include <signal.h> /* SIGCHLD, sigaction() */
#include <syslog.h>
#include <sys/types.h>  /* pid_t, u_long, u_short */
#include <sys/socket.h> /* struct sockaddr, socket(), etc */
#include <sys/wait.h>   /* waitpid(), WNOHAND */
#include <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
                        /* and INADDR_ANY */
#include "../stream.h"
#include "../netprotocol.h"
#include <dirent.h>
#include "../myftp/token.h"
#define SERV_TCP_PORT 8080 //default port

// Source: Chapter 8 Example 6 ser6.c
// claim as many zombies as we can
void claim_children();
// Source: Chapter 8 Example 6 ser6.c
// Turn server process into a daemon background process
void daemon_init(void);
// Source: chapter 8 Example 6 ser6.c
// Serve a client connecting to the server
void serve_a_client(int);
//server pwd function handler
void ser_pwd(int, char*); 
int main(int argc, char *argv[])
{
    int sd, nsd, n;
    pid_t pid;
    unsigned short port; //server listen port
    socklen_t cli_addrlen;
    struct sockaddr_in ser_addr, cli_addr;
    char dir[MAX_BLOCK_SIZE];
    //set the listening port to default port
    port = SERV_TCP_PORT;

    //set the initial directory of server.
    //if no directory provided use current directory
    if (argc == 1)
    {
        getcwd(dir, sizeof(dir));
    }
    //if directory provided use that directory
    else if (argc == 2)
    {
        strncpy(dir, argv[1], sizeof(dir));
    }
    //if more than 2 arg
    else
    {
        printf("Usage: %s [ initial_current_directory ]\n", argv[0]);
    }
    //check if dir is valid
    if (chdir(dir) < 0)
    {
        printf("Fail to set initial directory: %s", dir);
        exit(1);
    }
    //turn server into a daemon
    daemon_init();
    /* set up listening socket sd */
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("server:socket");
        exit(1);
    }

    /* build server Internet socket address */
    bzero((char *)&ser_addr, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_port = htons(port);
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* note: accept client request sent to any one of the
        network interface(s) on this host. 
     */

    /* bind server address to socket sd */
    if (bind(sd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0)
    {
        perror("server bind");
        exit(1);
    }

    /* become a listening socket */
    listen(sd, 5);
    while (1)
    {
        cli_addrlen = sizeof(cli_addr);
        nsd = accept(sd, (struct sockaddr *)&cli_addr, (socklen_t *)&cli_addrlen);
        if (nsd < 0)
        {
            if (errno == EINTR) /* if interrupted by SIGCHLD */
                continue;
            perror("server:accept");
            exit(1);
        }
        /* create a child process to handle this client */
        if ((pid = fork()) < 0)
        {
            perror("fork");
            exit(1);
        }
        else if (pid > 0)
        {
            close(nsd);
            continue; /* parent to wait for next client */
        }

        /* now in child, serve the current client */
        close(sd);
        serve_a_client(nsd);
        exit(0);
    }
}

void ser_pwd(int sd,char * buf)
{
    int len,nr,nw;
    char serverpath[MAX_BLOCK_SIZE];
    char status;
    buf[0] = PWD_CODE;
    getcwd(serverpath,MAX_BLOCK_SIZE);
    nr = strlen(serverpath);
    len = htons(nr);

    if(len > 0)
    {
        status = PWD_READY;
        bcopy(&len,&buf[2],4);

        nw = writen(sd, &buf[0], 1);
        nw = writen(sd, &status, 1);
        nw = writen(sd, &buf[2], 4);
        nw = writen(sd,serverpath,nr);
    }
    else
    {
        status = PWD_ERROR;
        nw = writen(sd, &status,1);
        return;
    }

}

void ser_dir(int sd, char * buf)
{
    int len,nw,nr;
    char status;
    buf[0] = DIR_CODE;

    DIR *dp;
    struct dirent *direntp;
    int filecount = 0;
    char files[MAX_NUM_TOKENS];
    char tmp[MAX_NUM_TOKENS];

    if ((dp = opendir(".")) == NULL)
    {
        printf("\tFailed to open directory\n");
        status = DIR_ERROR;
        nw = writen(sd, &status,1);
        return;
    }

    //get filenames
    while ((direntp = readdir(dp)) != NULL)
    {
        strcpy(tmp, direntp->d_name);

        if(tmp[0] != '.'){
            strcat(files, direntp->d_name);
            if(filecount != 0)
            {
                strcat(files, "\n\t");
            }
            // else
            // {
            //     strcat(files, "\t");
            // }
        }
        filecount++;
        if (filecount > MAX_NUM_TOKENS)
        {
            printf("\tToo many files to be displayed!\n");
            break;
        }

    }

    nr = strlen(files);
    len = htons(nr);
    bcopy(&len,&buf[2],4);

    if(nr == 0)
        status = DIR_ERROR;
    else
        status = DIR_READY;

    nw = writen(sd, &buf[0], 1);
    nw = writen(sd, &status, 1);
    nw = writen(sd, &buf[2], 4);
    nw = writen(sd, files, nr);
    return;
}

void claim_children()
{
    pid_t pid = 1;

    while (pid > 0)
    { /* claim as many zombies as we can */
        pid = waitpid(0, (int *)0, WNOHANG);
    }
}

void daemon_init(void)
{
    pid_t pid;
    struct sigaction act;

    if ((pid = fork()) < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid > 0)
    {
        printf("Hay, you'd better remember my PID: %d\n", pid);
        exit(0); /* parent goes bye-bye */
    }

    /* child continues */
    setsid(); /* become session leader */
    //chdir("/"); /* change working directory */
    umask(0); /* clear file mode creation mask */

    /* catch SIGCHLD to remove zombies from system */
    act.sa_handler = claim_children; /* use reliable signal */
    sigemptyset(&act.sa_mask);       /* not to block other signals */
    act.sa_flags = SA_NOCLDSTOP;     /* not catch stopped children */
    sigaction(SIGCHLD, (struct sigaction *)&act, (struct sigaction *)0);
}

void serve_a_client(int sd)
{
    int nw, nr;
    char buf[MAX_BLOCK_SIZE];
    char path[MAX_BLOCK_SIZE];
    char filename[MAX_BLOCK_SIZE];

    while (1)
    {
        bzero(buf, sizeof(buf));
        bzero(path, sizeof(path));
        bzero(filename, sizeof(filename));

        /*
        Read from client
        */
        if ((nr = readn(sd, buf, sizeof(buf))) <= 0)
        {
            return; //if failed to read
        }
        //process data
        if(buf[0] == PWD_CODE)
        {
            ser_pwd(sd,buf);
        }
        else if(buf[0] == DIR_CODE)
        {
            ser_dir(sd,buf);
        }
        //send back to client
        nw = writen(sd, buf, nr);
    }
}