#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	int ticks;

	if(argc!=1){
		fprintf(2,"Usage: uptime\n");
		exit(1);
	}

	ticks=uptime();
	fprintf(1,"%d\n",ticks);
	exit(0);
}
