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

    // copy file data
    int size = 0;
    int max = strlen(buffer);
    int i = 0;

    // inode pre-setup
    memset(inode_table + inode_num, 0, sizeof(struct ext2_inode));
    inode_table[inode_num].i_size = max;
    inode_table[inode_num].i_mode = EXT2_S_IFREG;
    inode_table[inode_num].i_links_count = 1;
    inode_table[inode_num].i_blocks = 0;

    unsigned int block_count = sb->s_blocks_count;

    // if the file is containable within the first 12 data blocks
    while(size < max && i < 12){
        int block_num = allocate_block(disk, bg, block_count);
        if (block_num == 0) {
            free(buffer);
            free(dest_inode);
            close(fd);
            return ENOMEM;
        }

        int block = new_block(sb, bg, disk, block_num);
        inode_table[inode_num].i_block[i] = block;
        inode_table[inode_num].i_blocks += 2;
        if((max - size) < EXT2_BLOCK_SIZE){
           unsigned char *dest = disk + block * EXT2_BLOCK_SIZE;
           memcpy(dest, buffer + size, strlen(buffer));
           break;
        } else {
           unsigned char *dest = disk + block * EXT2_BLOCK_SIZE;
           memcpy(dest, buffer + size, EXT2_BLOCK_SIZE);
           size += EXT2_BLOCK_SIZE;
           //buffer = buffer + EXT2_BLOCK_SIZE;
           i++;
        }
    }

        // if the file can't be contained within the first 12 data blocks
        // in this case, add one level of indirection
    if (i == 12 && size < max) {
        int block_num = allocate_block(disk, bg, block_count);
        if (block_num == 0) {
            free(buffer);
            free(dest_inode);
            close(fd);
            return ENOMEM;
        }
        int block = new_block(sb, bg, disk, block_num);

        inode_table[inode_num].i_block[12] = block;
        inode_table[inode_num].i_blocks += 2;

        unsigned int *indirect_block = (unsigned int *) BLOCK_PTR(block);

        for(int i = 0; i < EXT2_BLOCK_SIZE / 4; i++) {
            int block_num = allocate_block(disk, bg, block_count);
            if (block_num == 0) {
                free(buffer);
                free(dest_inode);
                close(fd);
                return ENOMEM;
            }
            int block = new_block(sb, bg, disk, block_num);

            *(indirect_block) = block;
            indirect_block += 1;

            if((max - size) < EXT2_BLOCK_SIZE){
                unsigned char *dest = disk + block * EXT2_BLOCK_SIZE;
                memcpy(dest, buffer + size, strlen(buffer));
                break;
            } else {
                unsigned char *dest = disk + block * EXT2_BLOCK_SIZE;
                memcpy(dest, buffer, EXT2_BLOCK_SIZE);
                size += EXT2_BLOCK_SIZE;
                //buffer = buffer + EXT2_BLOCK_SIZE;
            }
        }
      }

    free(buffer);
    free(dest_inode);
    close(fd);
    return 0;
}
