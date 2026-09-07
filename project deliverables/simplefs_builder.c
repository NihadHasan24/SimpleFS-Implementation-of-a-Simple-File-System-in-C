#include "simplefs.h"

static int write_at(FILE *fp, long offset, const void *buffer, size_t size)
{
    if (fseek(fp, offset, SEEK_SET) != 0) {
        return -1;
    }
    return fwrite(buffer, 1, size, fp) == size ? 0 : -1;
}

void set_bit(unsigned char *bitmap, int index) { bitmap[index / 8] |= (1u << (index % 8)); }
int is_bit_set(unsigned char *bitmap, int index) { return bitmap[index / 8] & (1u << (index % 8)); }
long inode_offset(int inode_number) { return ((long)INODE_TABLE_BLOCK * BLOCK_SIZE) + ((long)(inode_number - 1) * sizeof(inode_t)); }
int find_free_inode(unsigned char *bitmap) { (void)bitmap; return -1; }
int find_free_data_block(unsigned char *bitmap) { (void)bitmap; return -1; }

int main(int argc, char *argv[])
{
    char *image_name = NULL;
    FILE *fp;
    unsigned char zero_block[BLOCK_SIZE] = {0};
    unsigned char inode_bitmap[BLOCK_SIZE] = {0};
    unsigned char data_bitmap[BLOCK_SIZE] = {0};
    superblock_t sb;
    inode_t root_inode;
    dirent_t dot, dotdot;

    if (argc != 3) { printf("Usage: %s --image <image_name>\n", argv[0]); return 1; }
    if (strcmp(argv[1], "--image") != 0) { printf("Error: expected --image option.\n"); return 1; }
    image_name = argv[2];

    fp = fopen(image_name, "wb+");
    if (!fp) { printf("Error: could not create image file.\n"); return 1; }

    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        if (fwrite(zero_block, BLOCK_SIZE, 1, fp) != 1) { printf("Error: could not initialize image.\n"); fclose(fp); return 1; }
    }

    /* TODO 1: Fill all superblock fields. */
    memset(&sb, 0, sizeof(sb));
    /* TODO: STUDENT CODE START */
    sb.magic = MAGIC_NUMBER;
    sb.block_size = BLOCK_SIZE;
    sb.total_blocks = TOTAL_BLOCKS;
    sb.inode_count = TOTAL_INODES;
    sb.inode_bitmap_block = INODE_BITMAP_BLOCK;
    sb.data_bitmap_block = DATA_BITMAP_BLOCK;
    sb.inode_table_block = INODE_TABLE_BLOCK;
    sb.data_region_block = DATA_REGION_BLOCK;
    sb.root_inode = ROOT_INODE;
    /* TODO: STUDENT CODE END */
    if (write_at(fp, SUPERBLOCK_BLOCK * BLOCK_SIZE, &sb, sizeof(sb)) != 0) {
        printf("Error: could not write superblock.\n");
        fclose(fp);
        return 1;
    }

    /* TODO 2: Mark inode 1 allocated (inode bitmap index 0). */
    /* TODO: STUDENT CODE START */
    set_bit(inode_bitmap, ROOT_INODE - 1);
    /* TODO: STUDENT CODE END */
    if (write_at(fp, INODE_BITMAP_BLOCK * BLOCK_SIZE,
                 inode_bitmap, sizeof(inode_bitmap)) != 0) {
        printf("Error: could not write inode bitmap.\n");
        fclose(fp);
        return 1;
    }

    /* TODO 3: Mark root data block allocated (data bitmap index 0). */
    /* TODO: STUDENT CODE START */
    set_bit(data_bitmap, ROOT_DATA_BLOCK - DATA_REGION_BLOCK);
    /* TODO: STUDENT CODE END */
    if (write_at(fp, DATA_BITMAP_BLOCK * BLOCK_SIZE,
                 data_bitmap, sizeof(data_bitmap)) != 0) {
        printf("Error: could not write data bitmap.\n");
        fclose(fp);
        return 1;
    }

    /* TODO 4: Initialize root inode according to the specification. */
    memset(&root_inode, 0, sizeof(root_inode));
    /* TODO: STUDENT CODE START */
    root_inode.type = TYPE_DIRECTORY;
    root_inode.links = 2;
    root_inode.size = 2 * sizeof(dirent_t);
    root_inode.direct[0] = ROOT_DATA_BLOCK;
    /* TODO: STUDENT CODE END */
    if (write_at(fp, inode_offset(ROOT_INODE),
                 &root_inode, sizeof(root_inode)) != 0) {
        printf("Error: could not write root inode.\n");
        fclose(fp);
        return 1;
    }

    /* TODO 5: Initialize the '.' entry. */
    memset(&dot, 0, sizeof(dot));
    /* TODO: STUDENT CODE START */
    dot.inode_no = ROOT_INODE;
    dot.type = TYPE_DIRECTORY;
    strcpy(dot.name, ".");
    /* TODO: STUDENT CODE END */

    /* TODO 6: Initialize the '..' entry. */
    memset(&dotdot, 0, sizeof(dotdot));
    /* TODO: STUDENT CODE START */
    dotdot.inode_no = ROOT_INODE;
    dotdot.type = TYPE_DIRECTORY;
    strcpy(dotdot.name, "..");
    /* TODO: STUDENT CODE END */

    if (write_at(fp, ROOT_DATA_BLOCK * BLOCK_SIZE,
                 &dot, sizeof(dot)) != 0 ||
        write_at(fp, (ROOT_DATA_BLOCK * BLOCK_SIZE) + (long)sizeof(dot),
                 &dotdot, sizeof(dotdot)) != 0) {
        printf("Error: could not write root directory.\n");
        fclose(fp);
        return 1;
    }

    if (fclose(fp) != 0) {
        printf("Error: could not close image file safely.\n");
        return 1;
    }
    printf("SimpleFS image created successfully: %s\n", image_name);
    return 0;
}
