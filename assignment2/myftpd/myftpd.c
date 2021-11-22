/**
 * file:        myftpd.c
 * Author:      Seow Wei Cheng (33753618) and Jin Min Seok (33884206)
 * Date:        13/11/2021 (version 2)
 * Purpose:     This is the main driver code for the ftp server
 *              usage: myftpd [initial_current_directory]
 *              if no initial directory is provided current directory is assumed
 *              default port is 41314
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
#include <time.h>
#include <stdlib.h> /* strlen(), strcmp() etc */
#include <stdio.h>  /* printf()  */
#include <string.h> /* strlen(), strcmp() etc */
#include <errno.h>  /* extern int errno, EINTR, perror() */
#include <signal.h> /* SIGCHLD, sigaction() */
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>  /* pid_t, u_long, u_short */
#include <sys/socket.h> /* struct sockaddr, socket(), etc */
#include <sys/wait.h>   /* waitpid(), WNOHAND */
#include <netinet/in.h> /* struct sockaddr_in, htons(), htonl(), */
                        /* and INADDR_ANY */
#include "../stream.h"
#include "../netprotocol.h"
#include <dirent.h>
#include "../token.h"
#define SERV_TCP_PORT 41314 //default port

// Source: Chapter 8 Example 6 ser6.c
// claim as many zombies as we can
void claim_children();
// Source: Chapter 8 Example 6 ser6.c
// Turn server process into a daemon background process
void daemon_init(void);
// Source: chapter 8 Example 6 ser6.c
// Serve a client connecting to the server
void serve_a_client(int, char *);
//server pwd function handler
void ser_pwd(int, char *);
//server dir function handler
void ser_dir(int, char *);
//server put function handler
void ser_put(int, char *);
//server get function handler
void ser_get(int, char *);
//server cd function handler
void ser_cd(int, char *);
//function to log interaction with client
void log_file(char *, char *);

int main(int argc, char *argv[])
{
    int sd, nsd;
    pid_t pid;
    unsigned short port; //server listen port
    socklen_t cli_addrlen;
    struct sockaddr_in ser_addr, cli_addr;
    char dir[MAX_BLOCK_SIZE];
    //set the listening port to default port
    port = SERV_TCP_PORT;
    char log_path[MAX_BLOCK_SIZE];
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

    getcwd(dir, sizeof(dir));
    strcpy(log_path, dir);
    strcat(log_path, "/log.txt");
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
        serve_a_client(nsd, log_path);
        log_file("Client terminated session.\n", log_path);
        exit(0);
    }
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

void serve_a_client(int sd, char *log_path)
{
    int nr;
    char buf[MAX_BLOCK_SIZE];
    log_file("Client start session.", log_path);
    while (1)
    {
        bzero(buf, sizeof(buf));

        /*
        Read from client
        */
        if ((nr = readn(sd, buf, sizeof(buf))) <= 0)
        {
            return; //if failed to read
        }
        //process data
        if (buf[0] == PWD_CODE)
        {
            ser_pwd(sd, log_path);
        }
        else if (buf[0] == DIR_CODE)
        {
            ser_dir(sd, log_path);
        }
        else if (buf[0] == PUT_CODE1)
        {
            ser_put(sd, log_path);
        }
        else if (buf[0] == GET_CODE1)
        {
            ser_get(sd, log_path);
        }
        else if (buf[0] == CD_CODE)
        {
            ser_cd(sd, log_path);
        }
    }
}

void ser_pwd(int sd, char *log_path)
{
    int len, nr, nw;
    char serverpath[MAX_BLOCK_SIZE];
    char buf[MAX_BLOCK_SIZE];
    char status;
    buf[0] = PWD_CODE;
    getcwd(serverpath, MAX_BLOCK_SIZE);
    nr = strlen(serverpath);
    len = htons(nr);

    log_file("[pwd] pwd command received.", log_path);

    if (len > 0)
    {
        status = PWD_READY;
        bcopy(&len, &buf[2], 4);

        nw = writen(sd, &buf[0], 1);
        nw = writen(sd, &status, 1);
        nw = writen(sd, &buf[2], 4);
        nw = writen(sd, serverpath, nr);
        if (nw < 0)
        {
            log_file("[pwd] failed to write server response.", log_path);
        }
        log_file("[pwd] pwd function executed.", log_path);
    }
    else
    {
        status = PWD_ERROR;
        nw = writen(sd, &status, 1);
        log_file("[pwd] pwd function error.", log_path);
        return;
    }
    log_file("[pwd] pwd function ended.", log_path);
}

void ser_dir(int sd, char *log_path)
{
    char buf[MAX_BLOCK_SIZE];
    int len, nw, nr;
    char status;
    buf[0] = DIR_CODE;

    DIR *dp;
    struct dirent *direntp;
    int filecount = 0;
    char files[MAX_NUM_TOKENS];
    char tmp[MAX_NUM_TOKENS];

    log_file("[dir] dir command received.", log_path);

    if ((dp = opendir(".")) == NULL)
    {
        log_file("Failed to open directory.", log_path);
        status = DIR_ERROR;
        nw = writen(sd, &status, 1);
        return;
    }

    //get filenames
    while ((direntp = readdir(dp)) != NULL)
    {
        strcpy(tmp, direntp->d_name);

        if (tmp[0] != '.')
        {
            strcat(files, direntp->d_name);
            if (filecount != 0)
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
            log_file("Too many files to be displayed!", log_path);
            break;
        }
    }

    nr = strlen(files);
    len = htons(nr);
    bcopy(&len, &buf[2], 2);

    if (nr == 0)
        status = DIR_ERROR;
    else
        status = DIR_READY;

    nw = writen(sd, &buf[0], 1);
    nw = writen(sd, &status, 1);
    nw = writen(sd, &buf[2], 4);
    nw = writen(sd, files, nr);
    if (nw < 0)
    {
        log_file("[pwd] failed to write server response.", log_path);
    }
    log_file("[dir] function successfully executed.", log_path);
    return;
}

void ser_put(int sd, char *log_path)
{
    //variables used
    char opcode, ackcode;
    int file_len, fsize, nr, nw, fd, total = 0;
    char filename[MAX_BLOCK_SIZE]; //buffer to store filename
    char buf[MAX_BLOCK_SIZE];      //buffer to store client and server message
    //read file name length and convert to host byte order
    readn(sd, &buf[0], MAX_BLOCK_SIZE);
    memcpy(&file_len, &buf[0], 2);
    file_len = ntohs(file_len);
    //printf("file name length is: %d\n", file_len);
    log_file("[put] file name length received.", log_path);
    //read file name
    readn(sd, &buf[2], MAX_BLOCK_SIZE);
    memcpy(&filename, &buf[2], file_len);
    //set last index of filename to be NULL
    filename[file_len] = '\0';
    //printf("file name is: %s\n", filename);
    log_file("[put] file name received.", log_path);
    //check if file exist on server
    if (access(filename, R_OK) == 0)
    {
        ackcode = PUT_CLASH_ERROR;
        log_file("[put] put clash error.", log_path);
    }
    else
    {
        ackcode = PUT_READY;
        log_file("[put] put ready.", log_path);
    }
    //write opcode and ack code to client
    memset(buf, 0, MAX_BLOCK_SIZE);
    buf[0] = PUT_CODE1;
    buf[1] = ackcode;
    writen(sd, &buf[0], 1);
    writen(sd, &buf[1], 1);
    //if ackcode is '0'
    if (ackcode == PUT_READY)
    {
        //read opcode from client
        memset(buf, 0, MAX_BLOCK_SIZE);
        //check if can read opcode from client
        if (readn(sd, &buf[0], MAX_BLOCK_SIZE) < 0)
        {
            log_file("[put] failed to read opcode 2 from client", log_path);
            return;
        }
        memcpy(&opcode, &buf[0], 1);
        //printf("opcode is %c\n", opcode);
        log_file("[put] opcode 2 received.", log_path);
        //read file size
        //check if can read file size from client
        if (readn(sd, &buf[1], MAX_BLOCK_SIZE) < 0)
        {
            log_file("[put] failed to read file size from client", log_path);
            return;
        }
        memcpy(&fsize, &buf[1], 4);
        //convert file size to host byte order
        fsize = ntohl(fsize);
        //printf("file size is %d\n", fsize);
        log_file("[put] file size received.", log_path);
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
            //if total file size is smaller than max block size
            if (fsize < MAX_BLOCK_SIZE)
            {
                //read first block
                nr = readn(sd, block, MAX_BLOCK_SIZE);
                //if failed to read set ackcode to '1'
                if (nr < 0)
                {
                    log_file("[put] failed to read file.", log_path);
                    ackcode = PUT_FAIL;
                }
                //write block of data to file using leftover file size
                nw = write(fd, block, fsize);
                if (nw < 0)
                {
                    log_file("[put] failed to write file.", log_path);
                    ackcode = PUT_FAIL;
                }
            }
            else
            {
                //if total transfer is less than file size
                while (total < fsize)
                {
                    //reset block buffer
                    memset(block, '\0', MAX_BLOCK_SIZE);
                    //set file seek pointer to total transfer
                    lseek(fd, total, SEEK_SET);
                    //read next block of data
                    nr = readn(sd, block, MAX_BLOCK_SIZE);
                    if (nr < 0)
                    {
                        log_file("[put] failed to read file.", log_path);
                        ackcode = PUT_FAIL;
                    }
                    //if leftover data is less than max block size
                    int leftover = fsize - total;
                    if (leftover < MAX_BLOCK_SIZE)
                    {
                        //write to fd using the leftover file size
                        nw = write(fd, block, leftover);
                        if (nw < 0)
                        {
                            log_file("[put] failed to write file.", log_path);
                            ackcode = PUT_FAIL;
                        }
                    }
                    else
                    {
                        //write to fd using the max block size
                        nw = write(fd, block, MAX_BLOCK_SIZE);
                        if (nw < 0)
                        {
                            log_file("[put] failed to write file.", log_path);
                            ackcode = PUT_FAIL;
                        }
                    }
                    //add to total file count
                    total += nw;
                }
            }
            log_file("[put] file received from client.", log_path);
        }
        else
        {
            ackcode = PUT_FAIL;
            log_file("[put] put failed.", log_path);
        }
        //write to client status of file transfer
        memset(buf, 0, MAX_BLOCK_SIZE);
        opcode = PUT_CODE2;
        memcpy(&buf[0], &opcode, 1);
        //write opcode to client
        if (writen(sd, &buf[0], 1) < 0)
        {
            log_file("[put] Unable to send opcode to client.", log_path);
            return;
        }
        memcpy(&buf[1], &ackcode, 1);
        //write ack code to client
        if (writen(sd, &buf[1], 1) < 0)
        {
            log_file("[put] Unable to send ackcode to client.", log_path);
            return;
        }
        close(fd);
        log_file("[put] put command finished.", log_path);
        return;
    }
}

void ser_get(int sd, char *log_path)
{
    log_file("[get] get command received.", log_path);
    char opcode;
    int file_len, fsize, nr, total = 0;
    char filename[MAX_BLOCK_SIZE]; //buffer to store filename
    char buf[MAX_BLOCK_SIZE];      //buffer to store client and server message
    //read file name length and convert to host byte order
    readn(sd, &buf[0], MAX_BLOCK_SIZE);
    memcpy(&file_len, &buf[0], 2);
    file_len = ntohs(file_len);
    //printf("file name length is: %d\n", file_len);
    log_file("[get] file name length received.", log_path);
    //read file name
    readn(sd, &buf[2], MAX_BLOCK_SIZE);
    memcpy(&filename, &buf[2], file_len);
    //set last index of filename to be NULL
    filename[file_len] = '\0';
    //printf("file name is: %s\n", filename);
    log_file("[get] file name received.", log_path);
    FILE *file;                  //create file pointer
    file = fopen(filename, "r"); //open client selected file
    memset(buf, 0, MAX_BLOCK_SIZE);
    //check if file exist on server
    if (file != NULL)
    {
        buf[0] = GET_CODE1;
        buf[1] = GET_READY;
        if (writen(sd, &buf[0], 1) < 0)
        {
            log_file("[get] Error: failed to send opcode to client.", log_path);
            return;
        }
        if (writen(sd, &buf[1], 1) < 0)
        {
            log_file("[get] Error: failed to send ackcode to client.", log_path);
            return;
        }
        log_file("[get] File exist on server.", log_path);
        //get file size and send to client
        struct stat fst;
        //check if file stat is ok
        if (stat(filename, &fst) == -1)
        {
            log_file("[get] failed to get file stat.", log_path);
            return;
        }
        //get file size and convert it to network btye order
        memset(buf, 0, MAX_BLOCK_SIZE);
        opcode = GET_CODE2;
        memcpy(&buf[0], &opcode, 1);
        if (writen(sd, &buf[0], 1) < 0)
        {
            log_file("[get] failed to write opcode to client 2.", log_path);
        }
        fsize = (int)fst.st_size;
        int templen = htonl(fsize);
        memcpy(&buf[1], &templen, 4);
        if (writen(sd, &buf[1], 4) < 0)
        {
            log_file("[get] failed to write file size to client.", log_path);
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
        if (fsize < MAX_BLOCK_SIZE)
        {
            //read and write first block of data
            nr = read(fd, block, fsize);
            writen(sd, block, MAX_BLOCK_SIZE);
        }
        else
        {
            //if current sent block of data is smaller than total file size
            while (total < fsize)
            {
                //reset block buffer
                memset(block, '\0', MAX_BLOCK_SIZE);
                //set file seek pointer to previous end
                lseek(fd, total, SEEK_SET);

                //read next block of data
                int leftover = fsize - total;
                //if file size - current total size is larger than max block
                if (leftover > MAX_BLOCK_SIZE)
                {
                    //read next block of data to max block size
                    nr = read(fd, block, MAX_BLOCK_SIZE);
                }
                else
                {
                    //read next block of data to leftover size
                    nr = read(fd, block, leftover);
                }
                //read block data to server
                writen(sd, block, MAX_BLOCK_SIZE);
                //add write count to total size
                total += nr;
            }
        }
        log_file("[get] File is sent to client.", log_path);
    }
    else
    {
        buf[0] = GET_CODE1;
        buf[1] = GET_NOT_FOUND;
        if (writen(sd, &buf[0], 1) < 0)
        {
            log_file("[get] Error: failed to send opcode to client.", log_path);
            return;
        }
        if (writen(sd, &buf[1], 1) < 0)
        {
            log_file("[get] Error: failed to send ackcode to client.", log_path);
            return;
        }
        log_file("[get] File does not exist on server.", log_path);
        return;
    }
}

void ser_cd(int sd, char *log_path)
{
    log_file("[CD] CD command received.", log_path);
    char buf[MAX_BLOCK_SIZE];
    char path[MAX_BLOCK_SIZE];
    char status;
    int len;
    int chdirready;
    //read length from client
    if ((readn(sd, &buf[0], MAX_BLOCK_SIZE)) < 0)
    {
        log_file("[CD] failed to read file length.", log_path);
        return;
    }
    else
    {
        log_file("[CD] read file length.", log_path);
    }
    memcpy(&len, &buf[0], 2);
    len = ntohs(len);
    //printf("length is %d\n", len);
    //read filepath from client
    if ((readn(sd, &buf[2], MAX_BLOCK_SIZE)) < 0)
    {
        log_file("[CD] failed to read file path.", log_path);
        return;
    }
    else
    {
        log_file("[CD] read file path.", log_path);
    }
    memcpy(path, &buf[2], len);
    path[len] = '\0';

    chdirready = chdir(path);
    if (chdirready == 0)
    {
        status = CD_READY;
        //printf("[CD] status is ready.\n");
        log_file("[CD] status is ready.", log_path);
    }
    else if (chdirready == -1)
    {
        status = CD_ERROR;
        log_file("[CD] status is error.", log_path);
        //printf("[CD] status is error.\n");
    }
    memset(buf, 0, MAX_BLOCK_SIZE);
    buf[0] = CD_CODE;
    if ((writen(sd, &buf[0], 1)) < 0)
    {
        log_file("[CD] failed to write opcode to client.", log_path);
        return;
    }
    buf[1] = status;
    if ((writen(sd, &buf[1], 1)) < 0)
    {
        log_file("[CD] failed to write status to client.", log_path);
        return;
    }
    //printf("[CD] CD function ended.\n");
    log_file("[CD] CD function ended.", log_path);
    return;
}

void log_file(char *message, char *log_path)
{
    FILE *file;
    pid_t pid = getpid();
    char timearr[100];
    time_t raw_time;
    struct tm *time_info;

    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(timearr, sizeof(timearr), "%b %d %H:%M", time_info);
    file = fopen(log_path, "a");
    if (file == NULL)
    {
        perror("Server: log file.\n");
    }
    fprintf(file, "%d %s : ", pid, timearr);
    fprintf(file, "%s\n", message);
    fclose(file);
}
