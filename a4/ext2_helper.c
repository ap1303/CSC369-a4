#include "ext2.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define BLOCK_PTR(i) (disk + (i)*EXT2_BLOCK_SIZE)
#define INODE_PTR(i, inode_table) (inode_table[i - 1])

// allocate block; if there is no free blocks, return 0;
int allocate_block(unsigned char *disk, struct ext2_group_desc *bg, unsigned int blocks_count) {
    unsigned char *block_bitmap = disk + bg->bg_block_bitmap * EXT2_BLOCK_SIZE;
    int size = blocks_count / 8;
    for(int i = 0; i < size; i++) {
        unsigned char e = block_bitmap[i];
        for(int j = 0; j < 8; j++) {
            int temp = !!((e >> j) & 0x01);
            if (temp == 0) {
                block_bitmap[i] |= 1 << j;
                return i * 8 + j;
            }
        }
    }
    return 0;
}

// allocate block; if there is no free inodes, return 0;
int allocate_inode(unsigned char *disk, struct ext2_group_desc *bg, unsigned int inodes_count){
    unsigned char *inode_bitmap = disk + bg->bg_inode_bitmap * 1024;
    int inode_size = inodes_count / 8;
    for(int i = 0; i < inode_size; i++) {
        unsigned char e = inode_bitmap[i];
        for(int j = 0; j < 8; j++) {
            int temp = !!((e >> j) & 0x01);
            if (temp == 0) {
                inode_bitmap[i] |= 1 << j;
                return i * 8 + j;
            }
        }
   }
   return 0;
}

// search within a directory for a file or directory; if successful, return the
// inode; if not, return -1
int search_dir(unsigned char *disk, char *substring, struct ext2_inode *current) {
    struct ext2_dir_entry *dir = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE * current -> i_block[0]);
    int rec_len_sum = 0;
    int next_inode = -1;
    while (rec_len_sum < EXT2_BLOCK_SIZE) {
        dir = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE * current -> i_block[0] + rec_len_sum);
        unsigned int inode = dir -> inode;
        unsigned short rec_len = dir -> rec_len;
        char *name = dir -> name;
        if (strcmp(name, substring) == 0) {
            next_inode = inode;
            break;
        }
        rec_len_sum += rec_len;
    }
    if (next_inode == -1) {
        return -1;
    }
    return next_inode;
}

// Get the name of last entry in path, and the parent inode
int get_last_name(unsigned char *disk, struct ext2_inode *inode_table, struct ext2_inode *root, char *path, struct ext2_inode *parent_inode, char *name) {
     char *start = path + 1;
     char *end = strchr(start, '/');
     char *substring = malloc(sizeof(char) * 1024);
     if (end == NULL) {
         name = start;
         name[strlen(start)] = '\0';
         parent_inode[0] = *root;
     } else {
         strncpy(substring, start, end - start);
         substring[end - start] = '\0';
     }
     struct ext2_inode *current = root;
     while (end != NULL) {
         int inode = search_dir(disk, substring, current);
         if (inode == -1) {
             return ENOENT;
         }

         start = end;
         end = strchr(start + 1, '/');
         current = inode_table + (inode - 1);

         if (end == NULL) {
             strncpy(name, start + 1, strlen(start + 1));
             name[strlen(start + 1)] = '\0';
             parent_inode[0] = *current;
             break;
         } else {
             memset(substring, 0, 1024);
             strncpy(substring, start + 1, end - start - 1);
             substring[end - start] = '\0';
         }
      }
      return 0;
}

int reconfigure_dir(unsigned char *disk, struct ext2_inode parent, int current_block, char *name, int inode_num, int ftype) {
  // modify parent block to make room for the new one
  struct ext2_dir_entry *dir = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE * current_block);
  int rec_len_sum = 0;
  while (rec_len_sum < EXT2_BLOCK_SIZE) {
      dir = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE * current_block + rec_len_sum);
      unsigned short rec_len = dir -> rec_len;
      if (rec_len_sum + rec_len == EXT2_BLOCK_SIZE) {
          int raw_length = 4 + 2 + 1 + 1 + dir -> name_len;
          int padded_length = raw_length + (4 - raw_length % 4);
          rec_len_sum += padded_length;

          int remaining = EXT2_BLOCK_SIZE - rec_len_sum;
          if (remaining < 8 + strlen(name)) {
              return 1;
          }

          dir -> rec_len = padded_length;
          break;
      }
      rec_len_sum += rec_len;
  }
  
  dir = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE * parent.i_block[0] + rec_len_sum);
  dir -> inode = inode_num;
  dir -> rec_len = EXT2_BLOCK_SIZE - rec_len_sum;
  dir -> name_len = strlen(name);
  if (ftype == 0) {
      dir -> file_type = EXT2_FT_DIR;
  } else {
      dir -> file_type = EXT2_FT_REG_FILE;
  }
  strncpy(dir -> name, name, dir -> name_len);
  return 0; 
}

void explore_parent(struct ext2_inode *parent, unsigned char *disk, char *name, int inode) {
     for (int i = 0; i < 11; i++) {
        if (parent -> i_block[i] != 0 && parent -> i_block[i + 1] == 0) {
            int success = reconfigure_dir(disk, *parent, parent -> i_block[i], name, inode, 0);
            if (success == 1) {
                // not enough space on the last block
                parent -> i_block[i + 1] = inode;
                success = reconfigure_dir(disk, *parent, parent -> i_block[i+1], name, inode, 0);
            } else {
                // enough space on the last block
                // in this case, do nothing
            }    
        }
    }
}

int new_inode(struct ext2_super_block *sb, struct ext2_group_desc *bg, struct ext2_inode *inode_table, int inode_num){
    int inode = inode_num  + 1;

    memset(&inode_table[inode_num], 0, sizeof(struct ext2_inode));
    bg->bg_free_inodes_count--;
    sb->s_free_inodes_count--;

    return inode;
}

int new_block(struct ext2_super_block *sb, struct ext2_group_desc *bg, unsigned char *disk, int block_num){
    int block = block_num  + 1;

    memset(disk + block_num * EXT2_BLOCK_SIZE, 0, EXT2_BLOCK_SIZE);
    bg->bg_free_blocks_count--;
    sb->s_free_blocks_count--;

    return block;
}
