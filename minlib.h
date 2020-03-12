#ifndef MINLIBH
#define MINLIBH

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define DIR_MASK 0040000 /* Confirms an inode mode is directory */
#define FIL_MASK 0100000 /* Confirms an inode mode is file */
#define SYM_MASK 0120000 /* Confirms an inode mode is symlink */
#define O_R_MASK 0000400 /* Owner read permission */
#define O_W_MASK 0000200 /* Owner write permission */
#define O_X_MASK 0000100 /* Owner exec permission */
#define G_R_MASK 0000040 /* Group read permission */
#define G_W_MASK 0000020 /* Group write permission */
#define G_X_MASK 0000010 /* Group exec permission */
#define X_R_MASK 0000004 /* Other read permission */
#define X_W_MASK 0000002 /* Other write permission */
#define X_X_MASK 0000001 /* Other exec permission */
#define DIRECT_ZONES 7
#define MAGICOFFSET 510
#define MAGIC1 0x55
#define MAGIC2 0xAA
#define PTABLELOC 0x1BE
#define MINIXMAGIC 0x4D5A
#define SBOFFSET 1024
#define NAMELENGTH 60
#define NUMPART 4
#define MAXPATH 4097
#define SECTORSIZE 512
#define MINIXTYPE 0x81

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
void getPartitionTable(FILE *image, uintptr_t offset, uint32_t vflag, 
    Partition *partitiontable);
void printPartitionTable(Partition *partitiontable);
void getSuperBlock(FILE *image, uintptr_t offset, SuperBlock *superblock);
void printSuperblock(SuperBlock sb);
void getInode(FILE *image, uintptr_t offset, uint32_t index, Inode *inode);
void printInode(Inode inode);
uint32_t getZone(FILE* image, uint32_t indirectSize, Inode inode, 
    uint32_t index, Dirent* Zone, SuperBlock superblock, uintptr_t offset);
uint32_t checkZone(char* token, Dirent* Zone, uint32_t numEntries);
int checkDir(Inode inode);
void minls(Inode inode, char* path, FILE* image, SuperBlock sb, 
    uintptr_t po, uintptr_t io);
void lsdir(Inode inode, char* path, FILE* image, SuperBlock sb, 
    uintptr_t po, uintptr_t io);
void lsfile(Inode inode, char* path);
void printMask(Inode inode);
uint32_t checkDirent(Dirent dirent, char* token);
Dirent getDirent(FILE* image, Inode inode, SuperBlock sb, 
    uintptr_t offs, uint32_t index);
void minget(Inode inode, FILE* image, SuperBlock sb, 
    uintptr_t po, Options ops);
void safeRead(void *ptr, size_t size, size_t nmemb, FILE *stream);
void safeSeek(FILE *stream, long int offset, int whence);
void safeMalloc(void *ptr, size_t size);

#endif
