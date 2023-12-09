#include "kernel/types.h"
#include "user/user.h"
int isprime(int num)
{
	for(int i=2;i<num;i++)
	{
		if(num%i==0)
			{return 0;}
	}
	return 1;
}
	
void printprime(int fd[2],int maxnum)
{
	int prime=1;
	read(fd[0],&prime,sizeof(prime));
	do
	{
		prime++;
	}while(!isprime(prime));
	if(prime>maxnum)return;
	printf("prime %d\n",prime);
	write(fd[1],&prime,sizeof(prime));
	if(fork()==0)
	{
	printprime(fd,maxnum);
	}
	wait(0);
	return;
}

int main(int argc,char*argv[])
{
	if(argc!=1)
	{
		fprintf(2,"agrument exists\n");
		exit(1);	
	}
	int fd[2];
	pipe(fd);
	int initnum=1;
	write(fd[1],&initnum,sizeof(initnum));
	printprime(fd,35);
	close(fd[0]);
	close(fd[1]);
	exit(0);
	return 0;
}
