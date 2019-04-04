#include "ext2_helper.c"

int main(int argc, char **argv) {

    if(argc != 4) {
        fprintf(stderr, "Usage: cp <image file name> <path to source file> <path to dest> \n");
        exit(1);
    }

    // get the paths
    char source_path[strlen(argv[2]) + 1];
    char dest_path[strlen(argv[3]) + 1];

    memset(source_path, '\0', sizeof(source_path));
    memset(dest_path, '\0', sizeof(dest_path));

    strncpy(source_path, argv[2], strlen(argv[2]));
    strncpy(dest_path, argv[3], strlen(argv[3]));

    // zero out the trailing slash, if there is one.
    if (source_path[strlen(source_path) - 1] == '/') {
        source_path[strlen(source_path) - 1] = '\0';
    }
    if (dest_path[strlen(dest_path) - 1] == '/') {
        dest_path[strlen(dest_path) - 1] = '\0';
    }


    // read in file
    char *buffer = NULL;
    FILE *file = fopen(source_path, "r");
    if(file == NULL){
        return ENOENT;
    } else {
        fseek(file, 0, SEEK_END);
        int length = ftell(file);

        fseek(file, 0, SEEK_SET);
        buffer = malloc(length);

        fread(buffer, sizeof(char), length, file);
        fclose(file);
    }

    // read in the disk
    int fd = open(argv[1], O_RDWR);
	if(fd == -1) {
		free(buffer);
        return ENOENT;
    }

    unsigned char * disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        //perror("mmap");
        exit(1);
    }

    // decode destination path
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *) (disk + 2048);

    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *root = inode_table + (EXT2_ROOT_INO - 1);
    struct ext2_inode *dest_inode = malloc(sizeof(struct ext2_inode));
    char file_name[1024];

    int p_index = 0;
    int error = get_last_name(disk, inode_table, root, dest_path, dest_inode, file_name, &p_index);
    if (error == ENOENT) {
        //printf("get_last_name err\n");
        free(buffer);
        free(dest_inode);
        close(fd);
        return ENOENT;
    }
    struct ext2_dir_entry *ent = search_dir(disk, file_name, dest_inode);
    if (ent != NULL) {
        free(buffer);
        free(dest_inode);
        close(fd);
        return EEXIST;
    }

    // allocate new inode for the about-to-be-created file
    unsigned int inodes_count = sb->s_inodes_count;
    int inode_num = allocate_inode(disk, bg, inodes_count);
    if (inode_num == 0) {
        free(buffer);
        free(dest_inode);
        close(fd);
        return ENOMEM;
    }
    int n_inode = new_inode(sb, bg, inode_table, inode_num);

    // modify structure of the parent to create new dir entry for the file
    explore_parent(dest_inode, disk, file_name, n_inode, 1);

    unsigned int block_count = sb->s_blocks_count;

    write_to_data_blocks(buffer, inode_num, block_count, dest_inode, fd, sb, bg, inode_table, disk);

    free(buffer);
    free(dest_inode);
    close(fd);
    return 0;
}
