#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include "ext2_helper.c"

unsigned char *disk;

int main(int argc, char **argv) {
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
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

    unsigned int blocks_count = sb->s_blocks_count;
    int block_num = allocate_block(disk, bg, blocks_count);

    unsigned int inodes_count = sb->s_inodes_count;
    int inode_num = allocate_inode(disk, bg, inodes_count);

    // root node
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *root = inode_table + (EXT2_ROOT_INO - 1);

    // zero out the trailing slash, if there is one.
    char *path = argv[2];
    int length = strlen(path) - 1;
    if (path[strlen(path) - 1] == '/') {
        path[length] = '\0';
    }

    // traverse file path, see if there is discontinuities
    struct ext2_inode *parent = malloc(sizeof(struct ext2_inode));
    char name[1024];
    int discontinuities = get_last_name(disk, inode_table, root, path, parent, name);
    if (discontinuities == 0) {
        if (search_dir(disk, name, parent) != -1) {
            return EEXIST;
        }
    } else {
      return ENOENT;
    }

    // modify parent block to insert a new entry
    reconfigure_dir(disk, *parent, name, inode_num);

    // initialize inode for new entry
    inode_table[inode_num].i_mode = EXT2_S_IFDIR;
    inode_table[inode_num].i_size = 1024;
    inode_table[inode_num].i_blocks = 2;
    inode_table[inode_num].i_block[0] = block_num;
    inode_table[inode_num].i_links_count = 1;

    // create '.' for new inode
    struct ext2_dir_entry *self = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE * block_num);
    self -> inode = inode_num;
    self -> rec_len = 12;
    self -> name_len = 1;
    self -> file_type = EXT2_FT_DIR;
    self -> name[0] = '.';

    struct ext2_dir_entry *parent_pointer = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE * block_num + 12);
    parent_pointer -> inode = inode_num;
    parent_pointer -> rec_len = 12;
    parent_pointer -> name_len = 2;
    parent_pointer -> file_type = EXT2_FT_DIR;
    parent_pointer -> name[0] = '.';
    parent_pointer -> name[1] = '.';

    return 0;

}
