#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

int main(argc, char **argv) {

    if(argc != 4) {
        fprintf(stderr, "Usage: ext2_cp <image file name> <path to source file> <path to dest> \n", argv[0]);
        exit(1);
    }

    char sourcePath[strlen(argv[2]) + 1]
    char destPath[strlen(argv[3]) + 1]

    memset(sourcePath, '\0', sizeof(sourcePath));
    memset(destPath, '\0', sizeof(destPath));
    
    strncpy(sourcePath, argv[2], strlen(argv[2]));
    strncpy(destPath, argv[3], strlen(argv[3]));


    int fd = open(argv[1], O_RDWR);
	if(fd == -1) {
		perror("open");
		exit(1);
    }

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

}
