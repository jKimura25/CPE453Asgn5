#ifndef MINLIBH
#define MINLIBH

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define DIRECT_ZONES 7

typedef struct __attribute__ ((__packed__)) Options
{
    int32_t vflag;
    int32_t part;
    int32_t spart;  
    char* imgfile;
    char* srcpath;
    char* destpath;
} Options;

typedef struct __attribute__ ((__packed__)) Partition
{
    uint8_t bootind;
    uint8_t start_head;
    uint8_t start_sec;
    uint8_t start_cyl;
    uint8_t type;
    uint8_t end_head;
    uint8_t end_sec;
    uint8_t end_cyl;
    uint32_t lFirst;
    uint32_t size;
} Partition;

typedef struct __attribute__ ((__packed__)) SuperBlock 
{ 
    uint32_t ninodes;
    uint16_t pad1;
    int16_t i_blocks;
    int16_t z_blocks;
    uint16_t firstdata;
    int16_t log_zone_size;
    int16_t pad2;
    uint32_t max_file;
    uint32_t zones;
    int16_t magic;
    int16_t pad3;
    uint16_t blocksize;
    uint8_t subversion;
} SuperBlock;

typedef struct __attribute__ ((__packed__)) Inode 
{
    uint16_t mode;
    uint16_t links;
    uint16_t uid;
    uint16_t gid;
    uint32_t size;
    int32_t atime;
    int32_t mtime;
    int32_t ctime;
    uint32_t zone[DIRECT_ZONES];
    uint32_t indirect;
    uint32_t two_indirect;
    uint32_t unused;
} Inode;

typedef struct __attribute__ ((__packed__)) Dirent
{
    uint32_t inode;
    unsigned char name[60];
} Dirent;

void printHelp();
void parseOpts(int argc, char* argv[], Options *options);
void getPartitionTable(FILE *image, uintptr_t offset, uint32_t vflag, Partition *partitiontable);
void printPartitionTable(Partition *partitiontable);
void getSuperBlock(FILE *image, uintptr_t offset, SuperBlock *superblock);
void printSuperblock(SuperBlock sb);
void getInode(FILE *image, uintptr_t offset, uint32_t index, Inode *inode);
void printInode(Inode inode);
uint32_t getZone(FILE* image, uint32_t indirectSize, Inode inode, uint32_t index, Dirent* Zone, SuperBlock superblock);
uint32_t checkZone(char* token, Dirent* Zone, uint32_t numEntries);

#endif