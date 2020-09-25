#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	char p;
  char *xargv[MAXARG];
	int i,pos,ok;
	char buf[512];

	if(argc<2){
		fprintf(2,"Usage: xargs cmd args..\n");
		exit(1);
	}
	
	for(i=0;i<argc-1;i++){
		xargv[i]=argv[i+1];
	}
	// no args.
	if(argc==2){
		xargv[1]=0;
		if(fork()==0) exec(argv[1],xargv);
		else wait(0);
		exit(0);
	}

	pos=0;
	xargv[i]=buf;
	while(1){
		ok=read(0,&p,1);
		if((p=='\n'||ok==0)&&pos>0){
			xargv[i][pos]=0;
			if(fork()==0){
				exec(argv[1],xargv);
				exit(0);
			}else{
				wait(0);
			}
			pos=0;
		}else{
			xargv[i][pos++]=p;
		}
		if(ok==0) break;
	}
	exit(0);
}
