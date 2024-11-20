#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include<unistd.h>
#include<errno.h>
#include<stdio.h>
#include<stdlib.h>

char buffer[2048];

#define TRY_READ 10

int main(int argc, char *argv[])
{
	int fd;
	
	/*this variable holds remaining data bytes to be read */
	int remaining = TRY_READ;
	int seek	  = -1;

	/*Holds count of total data bytes read so far */
	int total_read=0;
	
	int n =0,ret=0;

	if(argc < 3){
		printf("Wrong usage\n");
		printf("Correct usage: <file> <readcount>\n");
		printf("Example ./dev_read.o /dev/pcd-0 10\n");
		return 0;
	}


	/*convert command line supplied data to integer */
	remaining = atoi(argv[2]);
	if(argc == 4){
		seek = atoi(argv[3]);
	}
	printf("read requested = %d\n",remaining);


	fd = open(argv[1],O_RDONLY);

	if(fd < 0){
		/*perror decodes user space errno variable and prints cause of failure*/
		perror("open");
		return fd;
	}

	printf("open was successful\n");


	/*activate this for lseek testing */
	if(seek != -1){
		printf("Seeking %d as SEEK_SET\n",seek);
		ret = lseek(fd,seek,SEEK_SET);
		if(ret < 0){
			perror("lseek");
			close(fd);
			return ret;
		}
	}

	/*Lets attempt reading twice */
	
	while(n != 2 && remaining)
	{
		/*read data from 'fd' */
		ret = read(fd,&buffer[total_read],remaining);

		if(!ret){
			/*There is nothing to read */
			printf("end of file \n");
			break;
		}else if(ret <= remaining){
			printf("read %d bytes of data \n",ret );
			/*'ret' contains count of data bytes successfully read , so add it to 'total_read' */
		        total_read += ret;
			/*We read some data, so decrement 'remaining'*/
			remaining -= ret;
		}else if(ret < 0){
			printf("something went wrong\n");
			break;
		}else
			break;

		n++;
	}

	printf("total_read = %d\n",total_read);

	//dump buffer
	printf("Data Read:\n");
	for(int i=0 ; i < total_read ; i++)
		printf("%c",buffer[i]);
	printf("\n");
	close(fd);
	
	return 0;
}
