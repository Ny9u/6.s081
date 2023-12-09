#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"
#define MAXSIZE 16
int main(int argc,char*argv[])
{
	//get input
	char buf[MAXSIZE];
	read(0,buf,MAXSIZE);
	
	//get command arguments
	int xargc=0;
	char *xargv[MAXARG];
	for(int i=1;i<argc;i++)
	{
		xargv[xargc++]=argv[i];
	}
	
	//exec
	char *p=buf;
	for(int j=0;j<MAXSIZE;j++)
	{
		if(buf[j]=='\n')
		{ 
			int pid=fork();
			if(pid==0)//son
			{
				buf[j]=0;
				xargv[xargc++]=p;
				xargv[xargc++]=0;
				exec(xargv[0],xargv);
				exit(0);	
			}else
			{
				p=&buf[j+1];
				wait(0);
			}
		}
	}
	wait(0);
	exit(0);

}
