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

    char source_path[strlen(argv[2]) + 1]
    char dest_path[strlen(argv[3]) + 1]

    memset(source_path, '\0', sizeof(source_path));
    memset(dest_path, '\0', sizeof(dest_path));

    strncpy(source_path, argv[2], strlen(argv[2]));
    strncpy(dest_path, argv[3], strlen(argv[3]));

    char *buffer = NULL;
    FILE *file = fopen(source_path, "r");
    if(file == NULL){
        perror("fopen: ");
        exit(-1);
    } else {
        fseek(file, 0, SEEK_END);
        int length = ftell(file);

        fseek(file, 0, SEEK_SET);
        buffer = malloc(length+1);

        fread(buffer, length, 1, f);
        fclose(file);
    }

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

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *) (disk + 2048);

    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *root = inode_table + (EXT2_ROOT_INO - 1);
    struct ext2_inode *dest_inode = malloc(sizeof(struct ext2_inode));
    char file_name[1024];

    int error = get_last_name(disk, inode_table, root, dest_path, dest_inode, file_name);
    if (error == ENOENT) {
        return ENOENT;
    }

    if (dest_inode && (dest_inode->i_mode & EXT2_S_IFDIR)) {
        int inode_num = allocate_inode(disk, bg, inodes_count);
        unsigned int inodes_count = sb->s_inodes_count;

        struct ext2_dir_entry *new_dir = malloc(sizeof(struct ext2_dir_entry));
        int new_inode = new_inode(sb, bg, inode_table, inode_num);
        if(new_inode < 0){
            free(dir);
            perror("No free inodes");
            return 0;
        }
        new_dir->inode = new_inode;
        new_dir->file_type = EXT2_FT_REG_FILE;
        strncpy(new_dir->name, file_name, strlen(file_name));
        new_dir->name_len = strlen(file_name);
        new_dir->rec_len = 0;

        // TODO add new dir to the parent inode


        int size = 0;
        int max = strlen(buffer);
        int i = 0;

        while( size < max){
            if( i == 11){
            }
            
            if( (max - size) < EXT2_BLOCK_SIZE){
            }

            i++;
        }
    }



}
