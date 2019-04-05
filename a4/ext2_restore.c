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

    init_resources(argv[1]);
    struct ext2_inode *root = inode_table + (EXT2_ROOT_INO - 1);
    struct ext2_inode *restore_parent_inode = malloc(sizeof(struct ext2_inode));
    char restore_name[1024];
    int p_index = 0;
    int error = get_last_name(disk, inode_table, root, rm_path, rm_parent_inode, rm_name, &p_index);
    if (error == ENOENT) {
        printf("get_last_name err\n");
        return ENOENT;
    }


    struct ext2_dir_entry* restore_dir;
    struct ext2_dir_entry* pre;

    int offset = find_restore_file(restore_dir, pre, restore_parent_inode, restore_name);

    int valid_restore = check_valid_restore(restore_dir, inode_table);
    if (valid_restore != 0) {
        return ENOENT;
    }

    restore_dir(&pre, &restore_dir, offset);
}
