#include <stdio.h>
#include <unistd.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

char* fileName;
int mainFD;
int totalGroups;
const int SUPER = 1024;

int SUCCESS_CODE = 0;
int ERR_CODE = 1;
int FAIL_CODE = 2;
int STDOUT = 1;

int SUPERBLOCK_OFFSET = 1024;
int SUPERBLOCK_SIZE = 1024;
int GROUP_DESC_SIZE = 32;
int INODE_SIZE = 128;
u_int32_t val = -1;

struct blockgroup {
    u_int32_t group_id;
    u_int32_t num_blocks;
    u_int32_t num_inodes;
    u_int16_t free_block_count;
    u_int16_t free_inodes_count;
    u_int32_t block_bitmap;
    u_int32_t inode_bitmap;
    u_int32_t inode_table;
};

struct inode {
    u_int32_t inode_num;
    char file_type;
    u_int16_t mode;
    u_int32_t owner;
    u_int32_t group;
    u_int16_t link_count;
    time_t time_since_inode_change;
    time_t modification_time;
    time_t time_last_access;
    u_int32_t file_size;
    u_int32_t num_blocks;
    u_int32_t offset;
    u_int32_t block_addresses[15];
};

u_int32_t block_count, inode_count, block_size, blocks_per_group, inodes_per_group, first_inode;
u_int16_t inode_size;




void print_gmt_time(time_t time)
{
    struct tm *curTime;
    
    curTime = gmtime(&time);
    
    
    char iNodeString[100];
    strftime(&iNodeString[0], 100, "%D %H:%M:%S", curTime);
    dprintf(STDOUT, "%s,", iNodeString);
}

void readError() {
    fprintf(stderr, "Error with pread. System Call Failed");
    exit(2);
}


// Extract inode data
void extract_inode_data(int img_fd, struct blockgroup* group_data,
                        u_int32_t num_block_groups, u_int8_t* inode_bitmaps, u_int32_t bytes_inode_bitmap,
                        struct inode* inode_data)
{
    unsigned int i;
    unsigned int j;
    unsigned int p;
    for (i = 0; i < num_block_groups; i++)
    {
        for (j = 0; j < bytes_inode_bitmap; j++)
        {
            u_int8_t bitmap = inode_bitmaps[i * bytes_inode_bitmap + j];
            for (p = 1; p <= 8; p++)
            {
                inode_data[(j * 8) + p - 1].inode_num = -1;
                
                int inode_allocated = 1 & bitmap;
                
                u_int16_t inode_mode = 0;
                pread(img_fd, &inode_mode, 2, (block_size * group_data[i].inode_table) + (((j * 8) + p - 1) * INODE_SIZE) + 0);
                
                u_int16_t link_count = 0;
                pread(img_fd, &link_count, 2, (block_size * group_data[i].inode_table) + (((j * 8) + p - 1) * INODE_SIZE) + 26);
                
                if (inode_allocated == 1 && inode_mode != 0 && link_count != 0)
                {
                    int index = (j * 8) + p - 1;
                    
                    // Offset
                    inode_data[index].offset = (block_size * group_data[i].inode_table) + (((j * 8) + p - 1) * INODE_SIZE);
                    
                    // Inode Number
                    inode_data[index].inode_num = (j * 8) + p;
                    
                    // File type
                    if (inode_mode & 0x8000)
                    {
                        inode_data[index].file_type = 'f';
                    }
                    else if (inode_mode & 0x4000)
                    {
                        inode_data[index].file_type = 'd';
                    }
                    else if (inode_mode & 0xA000)
                    {
                        inode_data[index].file_type = 's';
                    }
                    else
                    {
                        inode_data[index].file_type = '?';
                    }
                    
                    // Inode Mode
                    inode_data[index].mode = inode_mode;
                    
                    // Owner
                    u_int16_t uid;
                    pread(img_fd, &uid, 2, inode_data[index].offset + 2);
                    
                    u_int16_t uid_high;
                    pread(img_fd, &uid_high, 2, inode_data[index].offset + 116 + 4);
                    
                    inode_data[index].owner = (uid_high << 16) | uid;
                    
                    // Group
                    u_int16_t gid;
                    pread(img_fd, &gid, 2, inode_data[index].offset + 24);
                    
                    u_int16_t gid_high;
                    pread(img_fd, &gid_high, 2, inode_data[index].offset + 116 + 6);
                    
                    inode_data[index].group = (gid_high << 16) | gid;
                    
                    // Link Count
                    inode_data[index].link_count = link_count;
                    
                    // Time of last inode change
                    pread(img_fd, &inode_data[index].time_since_inode_change, 4, inode_data[index].offset + 12);
                    
                    // Modification Time
                    pread(img_fd, &inode_data[index].modification_time, 4, inode_data[index].offset + 16);
                    
                    // Time of last access
                    pread(img_fd, &inode_data[index].time_last_access, 4, inode_data[index].offset + 8);
                    
                    // File Size
                    pread(img_fd, &inode_data[index].file_size, 4, inode_data[index].offset + 4);
                    
                    // Number of blocks
                    pread(img_fd, &inode_data[index].num_blocks, 4, inode_data[index].offset + 28);
                    
                    // Block addresses
                    unsigned int a;
                    for (a = 0; a < 15; a++)
                    {
                        pread(img_fd, &inode_data[index].block_addresses[a], 4, inode_data[index].offset + 40 + (a * 4));
                    }
                }
                bitmap = bitmap >> 1;
            }
        }
    }
}

// INPUT: File system image file descriptor, superblock data, blockgroup data, number of groups
// Extract block group data
void extract_blockgroup_data(int img_fd,  struct blockgroup* group_data, u_int32_t num_block_groups)
{
    unsigned int remaining_inodes = inode_count;
    unsigned int remaining_blocks = block_count;
    
    unsigned int i;
    for (i = 0; i < num_block_groups; i++)
    {
        // Group number
        group_data[i].group_id = i;
        
        // Number of blocks in group
        if (remaining_blocks > blocks_per_group)
        {
            group_data[i].num_blocks = blocks_per_group;
            remaining_blocks -= blocks_per_group;
        }
        else
        {
            group_data[i].num_blocks = remaining_blocks;
        }
        
        // Number of inodes in group
        if (remaining_inodes > inodes_per_group)
        {
            group_data[i].num_inodes = inodes_per_group;
            remaining_inodes -= inodes_per_group;
        }
        else
        {
            group_data[i].num_inodes = remaining_inodes;
        }
        
        // Number of free blocks
        int bytes_read = pread(img_fd, &group_data[i].free_block_count, 2, SUPERBLOCK_OFFSET + SUPERBLOCK_SIZE + (i * GROUP_DESC_SIZE) + 12);
        if (bytes_read < 0)
        {
            readError();
        }
        
        // Number of free inodes
        bytes_read = pread(img_fd, &group_data[i].free_inodes_count, 2, SUPERBLOCK_OFFSET + SUPERBLOCK_SIZE + (i * GROUP_DESC_SIZE) + 14);
        if (bytes_read < 0)
        {
            readError();
        }
        
        // Block number of free block bitmap
        bytes_read = pread(img_fd, &group_data[i].block_bitmap, 4, SUPERBLOCK_OFFSET + SUPERBLOCK_SIZE + (i * GROUP_DESC_SIZE));
        if (bytes_read < 0)
        {
            readError();
        }
        
        // Block number of free inode bitmap
        bytes_read = pread(img_fd, &group_data[i].inode_bitmap, 4, SUPERBLOCK_OFFSET + SUPERBLOCK_SIZE + (i * GROUP_DESC_SIZE) + 4);
        if (bytes_read < 0)
        {
            readError();
        }
        
        // Block number of first block of inodes
        bytes_read = pread(img_fd, &group_data[i].inode_table, 4, SUPERBLOCK_OFFSET + SUPERBLOCK_SIZE + (i * GROUP_DESC_SIZE) + 8);
        if (bytes_read < 0)
        {
            readError();
        }
    }
}

void extract_inode_bitmaps(int img_fd,  struct blockgroup* group_data,
                           u_int32_t num_block_groups, u_int8_t* inode_bitmaps, u_int32_t bytes_inode_bitmap)
{
    unsigned int i;
    unsigned int j;
    for (i = 0; i < num_block_groups; i++)
    {
        for (j = 0; j < bytes_inode_bitmap; j++)
        {
            int bytes_read = pread(img_fd, &inode_bitmaps[i * bytes_inode_bitmap + j], 1, block_size * group_data[i].inode_bitmap + j);
            if (bytes_read < 0)
            {
                readError();
            }
        }
    }
}





int main(int argc, char** argv) {
    if (argc > 1) {
    fileName = argv[1];
    } else {
        fprintf(stderr, "No file system file given");
        exit(1);
    }
    if ((mainFD = open(fileName, O_RDONLY)) < 0) {
        fprintf(stderr, "Error opening fileSystem file");
        exit(1);
    }
    
    //Set up the values
    //Block count, number of inodes, block size, inode size, blocks per group, inodes per group,
   
   
    pread(mainFD, &block_count, 4, SUPER + 4);
    pread(mainFD, &inode_count, 4, SUPER);
    pread(mainFD, &block_size, 4, SUPER + 24);
    pread(mainFD, &inode_size, 2, SUPER + 88);
    pread(mainFD, &blocks_per_group, 4, SUPER + 32);
    pread(mainFD, &inodes_per_group, 4, SUPER + 40);
    pread(mainFD, &first_inode, 4, SUPER + 84);

    block_size = 1024 << block_size;
    
    
    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", block_count, inode_count, block_size, inode_size, blocks_per_group, inodes_per_group, first_inode);

    totalGroups = block_count / blocks_per_group;
    if (block_count != 0) {
        totalGroups++;
    }
    
    struct blockgroup* group_data = (struct blockgroup *) malloc(sizeof(struct blockgroup) * totalGroups);
    extract_blockgroup_data(mainFD, group_data, totalGroups);
    
     int i;
    for (i = 0; i < totalGroups; i++)
    {
        dprintf(STDOUT, "%s,", "GROUP");
        dprintf(STDOUT, "%d,", group_data[i].group_id);
        dprintf(STDOUT, "%d,", group_data[i].num_blocks);
        dprintf(STDOUT, "%d,", group_data[i].num_inodes);
        dprintf(STDOUT, "%d,", group_data[i].free_block_count);
        dprintf(STDOUT, "%d,", group_data[i].free_inodes_count);
        dprintf(STDOUT, "%d,", group_data[i].block_bitmap);
        dprintf(STDOUT, "%d,", group_data[i].inode_bitmap);
        dprintf(STDOUT, "%d\n", group_data[i].inode_table);
    }
    for (int i = 0; i < totalGroups; i++) {
        int offset = 2*SUPER + 32 * i;
        int blockOff;
        u_int16_t numFreeBlocks;
        //Get the block offset for the current group
        pread(mainFD, &numFreeBlocks, 2, offset + 12);

        pread(mainFD, &blockOff, 4, offset);
        u_int8_t map1;
        int counter = 1;
        for (u_int32_t k = 0; k < block_size; k++) {
            pread(mainFD, &map1, 1, k+ block_size * blockOff);
            for (int i = 0; i < 8; i++) {
                if (map1 & 1) {
                }
                else {
                    
                    printf("BFREE,%d\n", counter);
                    numFreeBlocks--;
                }
                
                counter++;
                map1 >>= 1;
        
            }
            if (numFreeBlocks == 0) {
                break;
            }
        }
        
    }
    
    // Free Inode Summary
    u_int32_t bytes_inode_bitmap = inodes_per_group / 8;
    u_int8_t* inode_bitmaps = (u_int8_t *) malloc(sizeof(u_int8_t) * totalGroups * bytes_inode_bitmap);
    
    extract_inode_bitmaps(mainFD, group_data, totalGroups,
                          inode_bitmaps, bytes_inode_bitmap);
    
    unsigned int num_nodes = 0;
    for (int i = 0; i < totalGroups; i++)
    {
        for (unsigned int j = 0; j < bytes_inode_bitmap; j++)
        {
            u_int8_t bitmap = inode_bitmaps[i * bytes_inode_bitmap + j];
            for (int p = 1; p <= 8; p++)
            {
                int inode_allocated = 1 & bitmap;
                if (inode_allocated == 0)
                {
                    dprintf(STDOUT, "%s,", "IFREE");
                    dprintf(STDOUT, "%d\n", (j * 8) + p);
                }
                else
                {
                    num_nodes++;
                }
                bitmap = bitmap >> 1;
            }
        }
    }
    
    
    
    
    // Inode Summary
    struct inode* inode_data = (struct inode *) malloc(sizeof(struct inode) * num_nodes);
    
    extract_inode_data(mainFD, group_data, totalGroups,
                       inode_bitmaps, bytes_inode_bitmap, inode_data);
    for (unsigned int i = 0; i < num_nodes; i++)
    {
        if (inode_data[i].inode_num != val)
        {
            dprintf(STDOUT, "%s,", "INODE");
            dprintf(STDOUT, "%d,", inode_data[i].inode_num);
            dprintf(STDOUT, "%c,", inode_data[i].file_type);
            dprintf(STDOUT, "%o,", inode_data[i].mode & 0xFFF);
            dprintf(STDOUT, "%d,", inode_data[i].owner);
            dprintf(STDOUT, "%d,", inode_data[i].group);
            dprintf(STDOUT, "%d,", inode_data[i].link_count);
            print_gmt_time(inode_data[i].time_since_inode_change);
            print_gmt_time(inode_data[i].modification_time);
            print_gmt_time(inode_data[i].time_last_access);
            dprintf(STDOUT, "%d,", inode_data[i].file_size);
            dprintf(STDOUT, "%d,", inode_data[i].num_blocks);
            int j;
            for (j = 0; j < 14; j++)
            {
                dprintf(STDOUT, "%d,", inode_data[i].block_addresses[j]);
            }
            dprintf(STDOUT, "%d\n", inode_data[i].block_addresses[14]);
        }
    }
    
    
    // Directory Entries
    for (unsigned i = 0; i < num_nodes; i++)
    {
        if (inode_data[i].inode_num != val && inode_data[i].file_type == 'd')
        {
            u_int32_t directory_file_block;
            pread(mainFD, &directory_file_block, 4, inode_data[i].offset + 40);
            
            u_int32_t curr_directory_entry_inode;
            pread(mainFD, &curr_directory_entry_inode, 4, directory_file_block * block_size + 0);
            
            u_int32_t directory_file_offset = 0;
            while (curr_directory_entry_inode != 0 && directory_file_offset < 1024)
            {
                u_int16_t entry_len;
                pread(mainFD, &entry_len, 2, directory_file_block * block_size + directory_file_offset + 4);
                
                u_int8_t name_len;
                pread(mainFD, &name_len, 1, directory_file_block * block_size + directory_file_offset + 6);
                
                u_int8_t name[255];
                memset(name, 0, sizeof(name));
                pread(mainFD, name, name_len, directory_file_block * block_size + directory_file_offset + 8);
                
                dprintf(STDOUT, "%s,", "DIRENT");
                dprintf(STDOUT, "%d,", inode_data[i].inode_num);
                dprintf(STDOUT, "%d,", directory_file_offset);
                dprintf(STDOUT, "%d,", curr_directory_entry_inode);
                dprintf(STDOUT, "%d,", entry_len);
                dprintf(STDOUT, "%d,", name_len);
                dprintf(STDOUT, "'%s'", name);
		dprintf(STDOUT, "\n"); 
                
                directory_file_offset += entry_len;
                pread(mainFD, &curr_directory_entry_inode, 4, directory_file_block * block_size + directory_file_offset + 0);
                
            }
        }
    }

    
    for (int i = 0; i < totalGroups; i++) {
        int offset = 2*SUPER + 32 * i;

        
        u_int32_t inodeTable;
        pread(mainFD, &inodeTable, 4, offset + 8);
        
        
        for (u_int32_t l = 0; l < inodes_per_group; l++) {
            int curOff = block_size * inodeTable + l * 128;
            u_int16_t linkCount;
            pread(mainFD, &linkCount, 2, curOff + 26);
            
            
            
            if (linkCount > 0) {
                
                for (int o = 12; o < 15; o++) {
                    u_int32_t indirectAddr, nextAddr, secondAddr, thirdAddr;
                    pread(mainFD, &indirectAddr, 4, curOff + 40 + 4 * o);
                    if (indirectAddr != 0) {
                        for (u_int32_t k = 0; k < block_size / 4; k++) {
                            pread(mainFD, &nextAddr, 4, indirectAddr * block_size + 4 * k);
                            if (nextAddr != 0) {
                                u_int32_t block1, block2, block3;
                                if (o == 12) {
                                block1 = 12 + k;
                                } else if (o == 13) {
                                    block1 = 12 + 256 + k;

                                } else {
                                    block1 = 12 + 256 + 256 * 256 + k;

                                }

                                printf("INDIRECT,%d,%d,%d,%d,%d\n", l+1, o - 11, block1, indirectAddr, nextAddr);
                                if (o > 12) {
                                    for (u_int32_t n = 0; n < block_size / 4; n++) {
                                        pread(mainFD, &secondAddr, 4, nextAddr * block_size + 4 * n);
                                        if (secondAddr != 0) {
                                            if (o == 13) {
                                                block2 = 12 + 256 + n;
                                            } else {
                                                block2 = 12 + 256 + 256 * 256+ n;
                                                
                                            }
                                            printf("INDIRECT,%d,%d,%d,%d,%d\n", l+1, o - 12, block2, nextAddr, secondAddr);
                                            
                                            if (o > 13) {
                                                for (u_int32_t m = 0; m < block_size / 4; m++) {
                                                    pread(mainFD, &thirdAddr, 4, secondAddr * block_size + 4 * m);
                                                    if (thirdAddr != 0) {
                                                        block3 =12 + 256 + 256 * 256 + m;
                                                        printf("INDIRECT,%d,%d,%d,%d,%d\n", l+1, o - 13, block3, secondAddr, thirdAddr);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                    }
                }
                
            }
        }
    }
     
    
    
    close(mainFD);
    
}
