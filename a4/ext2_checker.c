#include "ext2_helper.c"

unsigned char *disk;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ext2_checker <image file name>\n");
        exit(1);  
    }

    // read in the disk
    int fd = open(argv[1], O_RDWR);
	if(fd == -1) {
        exit(1);
    }

    unsigned char * disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        //perror("mmap");
        exit(1);
    }

    int total_fixes = 0;

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *) (disk + 2048);
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + bg -> bg_inode_table * EXT2_BLOCK_SIZE);
    unsigned char *block_bitmap = disk + bg -> bg_block_bitmap * EXT2_BLOCK_SIZE;
    unsigned char *inode_bitmap = disk + bg -> bg_inode_bitmap * EXT2_BLOCK_SIZE;


    fix_block_count(disk, sb -> s_blocks_count, sb, bg, &total_fixes);
    fix_inode_count(disk, sb -> s_inodes_count, sb, bg, &total_fixes);

    struct ext2_inode *root = inode_table + (EXT2_ROOT_INO - 1);
    fix_dir_files(disk, sb, bg, inode_bitmap, block_bitmap, inode_table, NULL, root, EXT2_ROOT_INO, &total_fixes);
}