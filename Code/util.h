#ifndef _UTIL_H
#define _UTIL_H

int readn(int fd,char* ptr,int nbytes);
int writen(int fd,char* ptr,int nbytes);
int readline(int fd,char* ptr,int maxlen);

#endif