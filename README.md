# SimpleFS:Implementation of a Simple File System in C

# SimpleFS
SimpleFS is the a small educational file system implemented in C and stored inside a fixed-size binary disk image. The project demonstrates superblocks, inodes, allocation bitmaps, directory entries, direct block pointers, and first-fit allocation.

## Project Files

| File | Purpose |
|---|---|
| `simplefs.h` | Fixed constants and on-disk structure definitions |
| `simplefs_builder.c` | Creates and initializes an empty SimpleFS image |
| `simplefs_adder.c` | Adds a regular file to an existing SimpleFS image |
| `test1.txt`, `test2.txt`, `test3.txt` | Sample input files |

## File-System Layout

SimpleFS uses 64 blocks of 4,096 bytes, producing a 262,144-byte (256 KiB)
image.

| Region | Blocks | Starting byte | Purpose |
|---|---:|---:|---|
| Superblock | 0 | 0 | Stores the file-system configuration |
| Inode bitmap | 1 | 4,096 | Tracks 32 inodes |
| Data bitmap | 2 | 8,192 | Tracks 60 data-region blocks |
| Inode table | 3 | 12,288 | Stores 32 inodes of 128 bytes each |
| Data region | 4–63 | 16,384 | Stores the root directory and file data |

The root directory always uses inode 1 and absolute data Block 4. Each file
inode contains three direct block pointers, limiting a file to 12,288 bytes.

## Features

- Creates a zero-initialized 256 KiB file-system image.
- Initializes the superblock, inode bitmap, data bitmap, and inode table.
- Creates the root inode and the `.` and `..` directory entries.
- Uses first-fit inode and data-block allocation.
- Supports zero-, one-, two-, and three-block files.
- Rejects duplicate names, oversized files, and names longer than 58 characters.
- Zero-fills the unused portion of a file's final data block.
- Checks relevant open, seek, read, write, and close operations.
- Reports errors safely without abnormal termination.

## Compilation

Compile from the project directory using GCC:

```bash
gcc -Wall -Wextra -std=c11 simplefs_builder.c -o simplefs_builder
gcc -Wall -Wextra -std=c11 simplefs_adder.c -o simplefs_adder
```

On Windows, GCC may create `simplefs_builder.exe` and `simplefs_adder.exe`.

## Usage

### Linux or Git Bash

Create a new image:

```bash
./simplefs_builder --image disk.img
```

Add files from the current working directory:

```bash
./simplefs_adder --input disk.img --file test1.txt
./simplefs_adder --input disk.img --file test2.txt
./simplefs_adder --input disk.img --file test3.txt
```

### Windows PowerShell

```powershell
.\simplefs_builder.exe --image disk.img
.\simplefs_adder.exe --input disk.img --file test1.txt
```

## Inspecting the Image

Use `xxd` to inspect important regions:

```bash
# Superblock
xxd -l 128 disk.img

# Inode bitmap
xxd -s 4096 -l 16 disk.img

# Data bitmap
xxd -s 8192 -l 16 disk.img

# Root directory
xxd -s 16384 -l 128 disk.img
```

Immediately after formatting, the inode and data bitmap outputs should both
begin with `01`. After adding the first small file, both should begin with `03`.

## Boundary Test

In Git Bash, create maximum-size and oversized files with:

```bash
dd if=/dev/zero of=maxfile.dat bs=1 count=12288 status=none
dd if=/dev/zero of=too_big.dat bs=1 count=12289 status=none

./simplefs_builder --image boundary.img
./simplefs_adder --input boundary.img --file maxfile.dat
./simplefs_adder --input boundary.img --file too_big.dat
```

The 12,288-byte file should succeed. The 12,289-byte file should be rejected.

## Error Handling

The programs handle:

- Missing or invalid command-line arguments
- Image creation and image-opening failures
- Missing, truncated, or invalid SimpleFS images
- Missing or unreadable source files
- Duplicate filenames
- Filenames longer than 58 characters
- Files larger than 12,288 bytes
- Exhausted inode or data-block space
- A full root directory
- Relevant seek, read, write, and close failures

## Known Limitations

- Only the root directory is supported.
- At most 31 regular files can be stored because inode 1 is reserved for root.
- Each file can use only three direct blocks.
- The maximum filename length is 58 characters.
- Source files must be in the current working directory.
- File deletion, renaming, indirect blocks, links, permissions, journaling,
  checksums, mounting, and caching are not implemented.
- No known correctness problems were found during normal, boundary,
  allocation-exhaustion, and error-handling tests.

