#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void swap(int* a,int* b,int size){
	int i;
	int temp;

	for(i=0;i<size;i++){
		temp=a[i];
		a[i]=b[i];
		b[i]=temp;
	}
}

int
main(int argc, char *argv[])
{
	int n;
	int prime=0,first=1;
	int parent[2],child[2];

	if(argc!=1){
		fprintf(2,"Usage: primes\n");
		exit(1);
	}
	
	pipe(parent);
	for(n=2;n<=35;n++){
		write(parent[1],&n,4);
	}
	close(parent[1]);
	printf("close %d\n",parent[1]);
	while(read(parent[0],&n,4)){
		if(first){
			prime=n;
			printf("prime %d\n",prime);
			pipe(child);
			if(fork()==0){
				swap(parent,child,2);
				// fork will create links to pipe file,
				// so we delete links don't need.
				close(parent[1]);
				close(child[0]);
			}else{
				first=0;
			}
		}else if(n%prime!=0){
			write(child[1],&n,4);
		}
	}
	close(parent[0]);
	close(child[1]);
	pipe(parent);
	wait(0);
	exit(0);
}
