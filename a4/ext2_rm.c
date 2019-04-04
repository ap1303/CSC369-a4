#include "ext2_helper.c"
#include <time.h>

int main(int argc, char **argv) {
    if(argc != 3){
        printf("Usage: ext2_rm <image file name> <path to link>\n");
        exit(1);
    }

    char rm_path[strlen(argv[2]) + 1];
    memset(rm_path, '\0', sizeof(rm_path));
    strncpy(rm_path, argv[2], strlen(argv[2]));

    int fd = open(argv[1], O_RDWR);
	if(fd == -1) {
		perror("open");
		exit(1);
    }

    unsigned char * disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *) (disk + 2048);
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *root = inode_table + (EXT2_ROOT_INO - 1);
    struct ext2_inode *rm_parent_inode = malloc(sizeof(struct ext2_inode));
    char rm_name[1024];
    int p_index = 0;

    int error = get_last_name(disk, inode_table, root, rm_path, rm_parent_inode, rm_name, &p_index);
    if (error == ENOENT) {
        printf("get_last_name err\n");
        return ENOENT;
    }

    struct ext2_dir_entry *r_dir = search_dir(disk, rm_name, rm_parent_inode);
    int inode_index = r_dir->inode;
    if (inode_index == -1) {
        printf("File doesn't exist");
        return ENOENT;
    }

    struct ext2_inode* rm_inode = &inode_table[inode_index - 1];
    int num_blocks = rm_inode->i_blocks / 2;

    int d_block = search_in_inode(disk , rm_inode, rm_name);
    if (d_block < 0) {
        return ENOENT;
    }

    rm_dir(disk, d_block, rm_name);

    rm_inode->i_links_count--;
    if(rm_inode->i_links_count > 0){
        return 0;
    }
    rm_inode->i_dtime = time(NULL);

    for(int i = 0; i < num_blocks; i++){
        free_block_map(disk, bg, sb, (rm_inode->i_block)[i]);
    }

    if (rm_inode->i_links_count == 0){
        free_inode_map(disk, bg, sb, inode_index);
    }
}
