#include "ext2_helper.c"
#include <time.h>

int main(int argc, char **argv) {
    if(argc != 3){
        printf("Usage: ext2_rm <image file name> <path to link>\n");
        exit(1);
    }

    char restore_path[strlen(argv[2]) + 1];
    memset(restore_path, '\0', sizeof(restore_path));
    strncpy(restore_path, argv[2], strlen(argv[2]));

    init_resources(argv[1]);
    struct ext2_inode *inode_table = (struct ext2_inode *) (disk + 1024 * bg->bg_inode_table);
    struct ext2_inode *root = inode_table + (EXT2_ROOT_INO - 1);
    struct ext2_inode *restore_parent_inode = malloc(sizeof(struct ext2_inode));
    char restore_name[1024];
    int p_index = 0;
    int error = get_last_name(disk, inode_table, root, restore_path, restore_parent_inode, restore_name, &p_index);
    if (error == ENOENT) {
        printf("get_last_name err\n");
        return ENOENT;
    }


    struct ext2_dir_entry* restore_dir = malloc(sizeof(struct ext2_dir_entry));
    struct ext2_dir_entry* pre = malloc(sizeof(struct ext2_dir_entry));

    int offset = find_restore_file(restore_dir, pre, restore_parent_inode, restore_name);

    int valid_restore = check_valid_restore(restore_dir, inode_table);
    if (valid_restore != 0) {
        return ENOENT;
    }

    restore_entry(pre, restore_dir, offset, inode_table);
}
