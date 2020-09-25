#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	int p1[2],p2[2];
	char buf[1];

	if(argc!=1){
		fprintf(2,"Usage: pingpong\n");
		exit(1);
	}

	pipe(p1);
	pipe(p2);
	if(fork()==0){
		read(p1[0],buf,1);
		printf("%d: received ping\n",getpid(),buf);
		write(p2[1],buf,1);
	}else{
		write(p1[1],buf,1);
		read(p2[0],buf,1);
		printf("%d: received pong\n",getpid(),buf);
	}
	exit(0);
}
