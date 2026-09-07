#include "simplefs.h"
#include <errno.h>

static int read_at(FILE *fp, long offset, void *buffer, size_t size)
{
    if (fseek(fp, offset, SEEK_SET) != 0) {
        return -1;
    }
    return fread(buffer, 1, size, fp) == size ? 0 : -1;
}

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
int data_bitmap_index(int absolute_block_number) { return absolute_block_number - DATA_REGION_BLOCK; }

int find_free_inode(unsigned char *bitmap)
{
    /* TODO 1: Search bitmap indexes 1..31 and return INODE NUMBER. */
    /* TODO: STUDENT CODE START */
    for (int index = 1; index < TOTAL_INODES; index++) {
        if (!is_bit_set(bitmap, index)) {
            return index + 1;
        }
    }
    /* TODO: STUDENT CODE END */
    return -1;
}

int find_free_data_block(unsigned char *bitmap)
{
    /* TODO 2: First-fit search; return ABSOLUTE data block number. */
    /* TODO: STUDENT CODE START */
    for (int index = 1; index < DATA_BLOCKS; index++) {
        if (!is_bit_set(bitmap, index)) {
            return DATA_REGION_BLOCK + index;
        }
    }
    /* TODO: STUDENT CODE END */
    return -1;
}

int filename_exists(FILE *image, const char *filename)
{
    dirent_t entry;
    /* TODO 3: Search root directory entries for filename. */
    /* TODO: STUDENT CODE START */
    for (int index = 2; index < BLOCK_SIZE / (int)sizeof(dirent_t); index++) {
        long position = ((long)ROOT_DATA_BLOCK * BLOCK_SIZE) +
                        ((long)index * sizeof(dirent_t));

        if (read_at(image, position, &entry, sizeof(entry)) != 0) {
            return -1;
        }

        if (entry.inode_no != 0 &&
            strncmp(entry.name, filename, sizeof(entry.name)) == 0) {
            return 1;
        }
    }
    /* TODO: STUDENT CODE END */
    return 0;
}

int find_free_directory_entry(FILE *image)
{
    dirent_t entry;
    /* TODO 4: Search entries 2..63; free entry has inode_no == 0. */
    /* TODO: STUDENT CODE START */
    for (int index = 2; index < BLOCK_SIZE / (int)sizeof(dirent_t); index++) {
        long position = ((long)ROOT_DATA_BLOCK * BLOCK_SIZE) +
                        ((long)index * sizeof(dirent_t));

        if (read_at(image, position, &entry, sizeof(entry)) != 0) {
            return -2;
        }

        if (entry.inode_no == 0) {
            return index;
        }
    }
    /* TODO: STUDENT CODE END */
    return -1;
}

int main(int argc, char *argv[])
{
    char *image_name = NULL, *source_name = NULL;
    FILE *image, *source;
    superblock_t sb;
    unsigned char inode_bitmap[BLOCK_SIZE], data_bitmap[BLOCK_SIZE];
    inode_t new_inode, root_inode;
    dirent_t new_entry;
    long file_size;
    long image_size;
    int required_blocks, free_inode, filename_status;
    int allocated_blocks[MAX_DIRECT_BLOCKS] = {0};
    int directory_entry_index;

    if (argc != 5) { printf("Usage: %s --input <image> --file <file>\n", argv[0]); return 1; }
    if (strcmp(argv[1], "--input") != 0 || strcmp(argv[3], "--file") != 0) { printf("Error: invalid command-line arguments.\n"); return 1; }
    image_name = argv[2]; source_name = argv[4];

    image = fopen(image_name, "rb+");
    if (!image) {
        if (errno == ENOENT) {
            printf("Error: file-system image not found.\n");
        } else {
            printf("Error: could not open file-system image for reading and writing.\n");
        }
        return 1;
    }
    if (read_at(image, SUPERBLOCK_BLOCK * BLOCK_SIZE, &sb, sizeof(sb)) != 0) { printf("Error: could not read superblock.\n"); fclose(image); return 1; }
    if (sb.magic != MAGIC_NUMBER) { printf("Error: invalid SimpleFS image.\n"); fclose(image); return 1; }
    if (fseek(image, 0, SEEK_END) != 0 ||
        (image_size = ftell(image)) < 0) {
        printf("Error: could not determine image size.\n");
        fclose(image);
        return 1;
    }
    if (image_size != (long)TOTAL_BLOCKS * BLOCK_SIZE) {
        printf("Error: invalid SimpleFS image size.\n");
        fclose(image);
        return 1;
    }

    source = fopen(source_name, "rb");
    if (!source) {
        if (errno == ENOENT) {
            printf("Error: source file not found.\n");
        } else {
            printf("Error: could not open source file for reading.\n");
        }
        fclose(image);
        return 1;
    }
    if (fseek(source, 0, SEEK_END) != 0 ||
        (file_size = ftell(source)) < 0 ||
        fseek(source, 0, SEEK_SET) != 0) {
        printf("Error: could not determine source file size.\n");
        fclose(source);
        fclose(image);
        return 1;
    }
    if (file_size > MAX_FILE_SIZE) { printf("Error: file is too large for SimpleFS.\n"); fclose(source); fclose(image); return 1; }

    /* TODO 5: Calculate required_blocks. Zero-byte file uses zero blocks. */
    /* TODO: STUDENT CODE START */
    if (strlen(source_name) > sizeof(new_entry.name) - 1) {
        printf("Error: file name is longer than 58 characters.\n");
        fclose(source);
        fclose(image);
        return 1;
    }
    required_blocks = (int)((file_size + BLOCK_SIZE - 1) / BLOCK_SIZE);
    /* TODO: STUDENT CODE END */

    filename_status = filename_exists(image, source_name);
    if (filename_status < 0) { printf("Error: could not read root directory.\n"); fclose(source); fclose(image); return 1; }
    if (filename_status > 0) { printf("Error: file already exists in SimpleFS.\n"); fclose(source); fclose(image); return 1; }

    if (read_at(image, INODE_BITMAP_BLOCK * BLOCK_SIZE,
                inode_bitmap, sizeof(inode_bitmap)) != 0) {
        printf("Error: could not read inode bitmap.\n");
        fclose(source);
        fclose(image);
        return 1;
    }
    free_inode = find_free_inode(inode_bitmap);
    if (free_inode == -1) { printf("Error: no free inode available.\n"); fclose(source); fclose(image); return 1; }

    if (read_at(image, DATA_BITMAP_BLOCK * BLOCK_SIZE,
                data_bitmap, sizeof(data_bitmap)) != 0) {
        printf("Error: could not read data bitmap.\n");
        fclose(source);
        fclose(image);
        return 1;
    }

    /* TODO 6: Allocate required data blocks and mark them in memory. */
    /* TODO: STUDENT CODE START */
    for (int i = 0; i < required_blocks; i++) {
        int block_number = find_free_data_block(data_bitmap);

        if (block_number == -1) {
            printf("Error: insufficient free data blocks.\n");
            fclose(source);
            fclose(image);
            return 1;
        }

        allocated_blocks[i] = block_number;
        set_bit(data_bitmap, data_bitmap_index(block_number));
    }
    /* TODO: STUDENT CODE END */

    directory_entry_index = find_free_directory_entry(image);
    if (directory_entry_index == -2) { printf("Error: could not read root directory.\n"); fclose(source); fclose(image); return 1; }
    if (directory_entry_index == -1) { printf("Error: root directory is full.\n"); fclose(source); fclose(image); return 1; }

    /* TODO 7: Copy source contents into allocated blocks using zero-filled buffers. */
    /* TODO: STUDENT CODE START */
    {
        unsigned char block_buffer[BLOCK_SIZE];
        long bytes_remaining = file_size;

        for (int i = 0; i < required_blocks; i++) {
            size_t bytes_to_read = bytes_remaining > BLOCK_SIZE
                                   ? BLOCK_SIZE
                                   : (size_t)bytes_remaining;

            memset(block_buffer, 0, sizeof(block_buffer));
            if (bytes_to_read > 0 &&
                fread(block_buffer, 1, bytes_to_read, source) != bytes_to_read) {
                printf("Error: could not read source file.\n");
                fclose(source);
                fclose(image);
                return 1;
            }

            if (write_at(image, (long)allocated_blocks[i] * BLOCK_SIZE,
                         block_buffer, sizeof(block_buffer)) != 0) {
                printf("Error: could not write file data to image.\n");
                fclose(source);
                fclose(image);
                return 1;
            }

            bytes_remaining -= (long)bytes_to_read;
        }
    }
    /* TODO: STUDENT CODE END */

    /* TODO 8: Initialize new file inode and its direct pointers. */
    memset(&new_inode, 0, sizeof(new_inode));
    /* TODO: STUDENT CODE START */
    new_inode.type = TYPE_FILE;
    new_inode.links = 1;
    new_inode.size = (uint32_t)file_size;
    for (int i = 0; i < required_blocks; i++) {
        new_inode.direct[i] = (uint32_t)allocated_blocks[i];
    }
    /* TODO: STUDENT CODE END */
    if (write_at(image, inode_offset(free_inode),
                 &new_inode, sizeof(new_inode)) != 0) {
        printf("Error: could not write file inode.\n");
        fclose(source);
        fclose(image);
        return 1;
    }

    /* TODO 9: Mark allocated inode in inode bitmap. */
    /* TODO: STUDENT CODE START */
    set_bit(inode_bitmap, free_inode - 1);
    /* TODO: STUDENT CODE END */
    if (write_at(image, INODE_BITMAP_BLOCK * BLOCK_SIZE,
                 inode_bitmap, sizeof(inode_bitmap)) != 0) {
        printf("Error: could not write inode bitmap.\n");
        fclose(source);
        fclose(image);
        return 1;
    }
    if (write_at(image, DATA_BITMAP_BLOCK * BLOCK_SIZE,
                 data_bitmap, sizeof(data_bitmap)) != 0) {
        printf("Error: could not write data bitmap.\n");
        fclose(source);
        fclose(image);
        return 1;
    }

    /* TODO 10: Create directory entry; ensure name is null-terminated. */
    memset(&new_entry, 0, sizeof(new_entry));
    /* TODO: STUDENT CODE START */
    new_entry.inode_no = (uint32_t)free_inode;
    new_entry.type = TYPE_FILE;
    strncpy(new_entry.name, source_name, sizeof(new_entry.name) - 1);
    new_entry.name[sizeof(new_entry.name) - 1] = '\0';
    /* TODO: STUDENT CODE END */
    {
        long pos = ((long)ROOT_DATA_BLOCK * BLOCK_SIZE) + ((long)directory_entry_index * sizeof(dirent_t));
        if (write_at(image, pos, &new_entry, sizeof(new_entry)) != 0) {
            printf("Error: could not write directory entry.\n");
            fclose(source);
            fclose(image);
            return 1;
        }
    }

    if (read_at(image, inode_offset(ROOT_INODE),
                &root_inode, sizeof(root_inode)) != 0) {
        printf("Error: could not read root inode.\n");
        fclose(source);
        fclose(image);
        return 1;
    }

    /* TODO 11: Increase root_inode.size by sizeof(dirent_t). */
    /* TODO: STUDENT CODE START */
    root_inode.size += sizeof(dirent_t);
    /* TODO: STUDENT CODE END */
    if (write_at(image, inode_offset(ROOT_INODE),
                 &root_inode, sizeof(root_inode)) != 0) {
        printf("Error: could not update root inode.\n");
        fclose(source);
        fclose(image);
        return 1;
    }

    {
        int source_close_failed = fclose(source) != 0;
        int image_close_failed = fclose(image) != 0;
        if (source_close_failed || image_close_failed) {
            printf("Error: could not close files safely.\n");
            return 1;
        }
    }
    printf("%s added successfully to %s\n", source_name, image_name);
    return 0;
}
