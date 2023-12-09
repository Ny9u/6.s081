#include "kernel/types.h"
#include "user/user.h"
int
main(int argc,char*argv[])
{
	if(argc!=1)
	{
		fprintf(2,"too much agrument\n");
		exit(1);
	}
	int fd1[2];
	int fd2[2];
	pipe(fd1);
	pipe(fd2);
	int pid=fork();
	if(pid==0)
	{
		close(fd1[1]);
		close(fd2[0]);
		char buf[10];
		read(fd1[0],buf,4);
		printf("%d:received %s\n",getpid(),buf);
		close(fd1[0]);
		write(fd2[1],"pong",4);
		close(fd2[1]);
	}
	else
	{
		close(fd2[1]);
		close(fd1[0]);
		write(fd1[1],"ping",4);
		close(fd1[1]);
		char buf[10];
		read(fd2[0],buf,4);
		printf("%d:received %s\n",getpid(),buf);
		close(fd2[0]);
		
	}
	exit(0);
}
