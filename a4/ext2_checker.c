#include "ext2_helper.c"

unsigned char *disk;

int fix_inode_count(unsigned int inodes_count, struct ext2_super_block *sb, struct ext2_group_desc *bg, int *total_fixes) {
    unsigned char *inode_bitmap = disk + bg->bg_inode_bitmap * EXT2_BLOCK_SIZE;
    int size = inodes_count / 8;
    int i = 0;
    for(int i = 0; i < size; i++) {
        unsigned char e = inode_bitmap[i];
        for(int j = 0; j < 8; j++) {
            int temp = !!((e >> j) & 0x01);
            if (temp == 1) {
                i += 1;
            }
        }
    }

    int free_inodes = inodes_count - i;

    if (sb -> s_free_inodes_count != free_inodes) {
        int delta = sb -> s_free_inodes_count - free_inodes;
        sb -> s_free_inodes_count = free_inodes;
        printf("superblock's free inode counter was off by %d compared to bitmap\n", delta);
        *total_fixes += 1;
    }

    if (bg -> bg_free_inodes_count != free_inodes) {
        int delta = bg -> s_free_inodes_count - free_inodes;
        bg -> s_free_inodes_count = free_inodes;
        printf("block group's free inode counter was off by %d compared to bitmap\n", delta);
        *total_fixes += 1;
    }
    return 0;
}

int fix_block_count(unsigned int block_count, struct ext2_super_block *sb, struct ext2_group_desc *bg, int *total_fixes) {
    unsigned char *block_bitmap = disk + bg->bg_block_bitmap * EXT2_BLOCK_SIZE;
    int size = block_count / 8;
    int i = 0;
    for(int i = 0; i < size; i++) {
        unsigned char e = block_bitmap[i];
        for(int j = 0; j < 8; j++) {
            int temp = !!((e >> j) & 0x01);
            if (temp == 1) {
                i += 1;
            }
        }
    }

    int free_blocks = block_count - i;

    if (sb -> s_free_blocks_count != free_blocks) {
        int delta = sb -> s_free_blocks_count - free_blocks;
        sb -> s_free_blocks_count = free_blocks;
        printf("superblock's free blocks counter was off by %d compared to bitmap\n", delta);
        *total_fixes += 1;
    }

    if (bg -> bg_free_inodes_count != free_inodes) {
        int delta = bg -> s_free_blocks_count - free_blocks;
        bg -> s_free_blocks_count = free_blocks;
        printf("block group's free blocks counter was off by %d compared to bitmap\n", delta);
        *total_fixes += 1;
    }
    return 0;
}

int check_bitmap(struct ext2_super_block *sb, struct ext2_group_desc *bg, unsigned char *bitmap, int num, int inode, int *total) {
    int i = num / 8;
    int j = num - (8 * i);
    unsigned char e = bitmap[i + 1];
    if (e & (1 << j) == 0) {
        bitmap[i + 1] != (1 << j);
        if (inode == 1) {
            sb -> s_free_inodes_count += 1;
            bg -> bg_free_inodes_count += 1;
            printf("Fixed: inode [%d] not marked as in-use", num + 1);
            *total += 1; 
        } else {
            sb -> s_free_blocks_count += 1;
            bg -> bg_free_blocks_count += 1;
            *total += 1;
        }
        return 1;
    } else {
        // marked as in-use
        // in this case, do nothing
        return 0;
    }
}

void fix_file(struct ext2_super_block *sb, struct ext2_group_desc *bg, unsigned char *inode_bitmap, unsigned char *block_bitmap, struct ext2_dir_entry *dir, struct ext2_inode *inode, int inode_num, int *total) {
    if (inode -> i_mode & EXT2_S_IFREG == 0) {
        if (inode -> i_mode & EXT2_S_IFDIR) {
            dir -> file_type = EXT2_FT_DIR;
        } else {
            dir -> file_type = EXT2_FT_SYMLINK;
        }
        printf("Fixed: Entry type vs inode mismatch: inode [%d]", inode_num + 1);
        *total += 1;
    }
    check_bitmap(sb, bg, inode_bitmap, inode_num, 1, total);
    if (inode -> i_dtime != 0) {
        inode -> i_dtime = 0;
        printf("Fixed: valid inode marked for deletion: [%d]", inode_num + 1)；
        *total += 1;
    }
    int count = 0;
    for(int i = 0; i < 15; i++) {
        int block = inode.i_block[i]
        if (block != 0) {
            int success = check_bitmap(sb, bg, block_bitmap, block - 1, 0, total);
            if (success == 1) {
                count += 1;
            }
        }
    } 
    printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]", count, inode_num + 1);
}

void fix_dir_files(struct ext2_super_block *sb, struct ext2_group_desc *bg, unsigned char *inode_bitmap, unsigned char *block_bitmap, struct ext2_dir_entry *dir, struct ext2_inode *inode, int inode_num, int *total) {
     if (inode -> i_mode & EXT2_S_IFDIR == 0) {
        if (inode -> i_mode & EXT2_S_IFREG) {
            dir -> file_type = EXT2_FT_REG_FILE;
        } else {
            dir -> file_type = EXT2_FT_SYMLINK;
        }
        printf("Fixed: Entry type vs inode mismatch: inode [%d]", inode_num + 1);
        *total += 1;
    }
    check_bitmap(sb, bg, inode_bitmap, inode_num, 1, total);
    if (inode -> i_dtime != 0) {
        inode -> i_dtime = 0;
        printf("Fixed: valid inode marked for deletion: [%d]", inode_num + 1)；
        *total += 1;
    }
    int count = 0;
    for(int i = 0; i < 15; i++) {
        int block = inode -> i_block[i];
        if (block != 0) {
            int success = check_bitmap(sb, bg, block_bitmap, block - 1, 0, total);
            if (success == 1) {
                count += 1;
            }
        } else {
            break;
        }  
    } 
    printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]", count, inode_num + 1);

    // recursive calls
    for(int i = 0; i < 12; i++) {
        int block = inode -> inode[i];
        if (block != 0) {
            int rec_len_sum = 0;
            while (rec_len_sum )
            struct ext2_dir_entry *sub = (struct ext2_dir_entry *) (BLOCK_PTR(block) + rec_len_sum);
            
        } else {
            break;
        }
    }

}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ext2_checker <image file name>\n");
        exit(1);  
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

    int total_fixes = 0;

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *) (disk + 2048);
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * bg->bg_inode_table);

    fix_block_count(sb -> s_blocks_count, sb, bg, &total_fixes);
    fix_inode_count(sb -> s_inodes_count, sb, bg, &total_fixes);

    struct ext2_inode *root = inode_table[EXT2_ROOT_INO - 1];
    for(int i = 0; i < 12; i++) {
        int block = root -> i_block[i]
        if (block != 0) {
            int rec_len_sum = 0
            while (rec_len_sum < EXT2_BLOCK_SIZE) {
               struct ext2_dir_entry *dir = (struct ext2_dir_entry *) (BLOCK_PTR(block) + rec_len_sum);
               if (dir -> file_type == EXT2_FT_REG_FILE) {
                   fix_regular_files();
               } else if (dir -> file_type == EXT2_FT_SYMLINK) {
                   fix_symlink_files();
               } else {
                   fix_directory_files();
               }
               rec_len_sum += dir -> rec_len;
            } 
        } else {
            break;
        } 
    } 
}