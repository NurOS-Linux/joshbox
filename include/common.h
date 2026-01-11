#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>

#define MAX_ENTRIES 1024
#define PATH_BUFFER 4096
#define NAME_BUFFER 256

#define FLAG_LONG   0x01
#define FLAG_ALL    0x02
#define FLAG_HUMAN  0x04

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int cmd_ls(int argc, char **argv);
int cmd_cp(int argc, char **argv);
int cmd_mv(int argc, char **argv);

#endif
