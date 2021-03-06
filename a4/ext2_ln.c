#include "ext2_helper.c"

unsigned char *disk; 

int main(int argc, char **argv)  {
    if(argc != 4 && argc != 5) {
        fprintf(stderr, "Usage: %s <image file name> [-s] <source path> <destination path>\n", argv[0]);
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);
	if(fd == -1) {
		exit(1);
    }

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *) (disk + 2048);
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * bg->bg_inode_table);

    char *source_path = NULL;
    char *dest_path = NULL;
    struct ext2_inode *source_parent = malloc(sizeof(struct ext2_inode));
    char source_name[1024];
    struct ext2_inode *dest_parent = malloc(sizeof(struct ext2_inode));
    char dest_name[1024];
    int target_ft;
    if (argc == 5) {
        // symlink
        source_path = argv[3];
        dest_path = argv[4];
        target_ft = EXT2_FT_SYMLINK;
    } else {
        // hard links
        source_path = argv[2];
        dest_path = argv[3];
        target_ft = EXT2_FT_REG_FILE;
    }
    int index = 0;
    int dest_index = 0;

    // decode source path
    int success = get_last_name(disk, inode_table, inode_table + 1, source_path, source_parent, source_name, &index);
    if (success == ENOENT) {
        return ENOENT;
    }
    struct ext2_dir_entry *ent = search_dir(disk, source_name, source_parent);
    if (ent == NULL) {
        return ENOENT;
    }
    if (ent -> file_type == EXT2_FT_DIR && target_ft == EXT2_FT_REG_FILE) {
        return EISDIR;
    } 

    // decode destination path
    int dest_success = get_last_name(disk, inode_table, inode_table + 1, dest_path, dest_parent, dest_name, &dest_index);
    if (dest_success == ENOENT) {
        return ENOENT;
    }
    struct ext2_dir_entry *dest_ent = search_dir(disk, dest_name, dest_parent);
    if (dest_ent != NULL) {
        return EEXIST;
    }

    struct ext2_inode *parent = inode_table + (dest_index - 1);

    if (argc == 4) {
        // hard links
        explore_parent(parent, disk, dest_name, ent -> inode, 1);

        struct ext2_inode *source_parent = inode_table + (index - 1);
        source_parent -> i_links_count += 1;
    } else {
        // soft links
        int inode_num = allocate_inode(disk, bg, sb -> s_inodes_count);
        if (inode_num == 0) {
            return ENOMEM;
        }
        int inode = new_inode(sb, bg, inode_table, inode_num);

        // append to parent 
        explore_parent(parent, disk, dest_name, inode, 2);

        unsigned int block_count = sb->s_blocks_count;

        write_to_data_blocks(source_path, inode_num, block_count, parent, -1, sb, bg, inode_table, disk);

    }    
}

