#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include "minlib.h"

#define DIR_MASK 0040000 /* Confirms an inode mode is directory */
#define FIL_MASK 0100000 /* Confirms an inode mode is file */
#define O_R_MASK 0000400 /* Owner read permission */
#define O_W_MASK 0000200 /* Owner write permission */
#define O_X_MASK 0000100 /* Owner exec permission */
#define G_R_MASK 0000040 /* Group read permission */
#define G_W_MASK 0000020 /* Group write permission */
#define G_X_MASK 0000010 /* Group exec permission */
#define X_R_MASK 0000004 /* Other read permission */
#define X_W_MASK 0000002 /* Other write permission */
#define X_X_MASK 0000001 /* Other exec permission */

void printHelp()
{
    perror("usage: minls [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
    perror("Options:\n");
    perror("-p part    --- select partition for ");
    perror("filesystem (default: none)\n");
    perror("-s sub     --- select subpartition for ");
    perror("filesystem (default: none)\n");
    perror("-h help    --- print usage information and exit\n");
    perror("-v verbose --- increase verbosity level\n");
}

void parseOpts(int argc, char* argv[], Options *options)
{
    int c;

    options->vflag = 0;
    options->part = -1;
    options->spart = -1;  
    options->imgfile = NULL;
    options->srcpath = NULL;
    options->destpath = NULL;

    if(argc == 1)
    {
        printHelp();
        exit(0);
    }

    while((c = getopt(argc, argv, "hvp:s:")) != -1)
    {
        switch (c)
        {
            case 'h':
                printHelp();
                exit(-1);
            case 'v':
                options->vflag = 1;
                break;
            case 'p':
                options->part = atoi(optarg);
                break;
            case 's':
                options->spart = atoi(optarg);
                break;
            default:
                printf("Unknown command: %c\n", optopt);
        }
    }

    options->imgfile = argv[optind++];
    if (argv[optind] != NULL)
    {
        options->srcpath = argv[optind++];
        if (argv[optind] != NULL)
        {
            options->destpath = argv[optind];
        }
    }
}

void getPartitionTable(FILE *image, uintptr_t offset, uint32_t vflag, 
    Partition* partitiontable)
{   
    uint8_t magic[2];

    /* Go to partition table signature */
    if (-1 == fseek(image, offset + 510, SEEK_SET))
    {
        perror("partition seek failed!");
        exit(-1);
    }

    /* Read partition table signature */
    if(fread(magic, sizeof(uint8_t), 2, image) != 2)
    {
        perror("valid read failed!");
        exit(-1);
    }

    /* Validate partition signature */
    if(magic[0] != 0x55 || magic[1] != 0xAA)
    {
        printf("Invalid partition table!\n");
        exit(-1);
    }

    /* Go to partition table */
    if (-1 == fseek(image, offset + 0x1BE, SEEK_SET))
    {
        perror("fseek failed!");
        exit(-1);
    }

    /* Read partition table */
    if(fread(partitiontable, sizeof(Partition), 4, image) != 4)
    {
        perror("partition read failed!");
        exit(-1);
    }
}

void printPartitionTable(Partition *partitiontable)
{
    int i;
    printf("       ----Start----      ------End-----\n");
    printf("  Boot head  sec  cyl Type head  sec  cyl      First       Size\n");
    for(i = 0; i < 4; i++)
    {
        printf("  %#4x", partitiontable[i].bootind);
        printf(" %4d", partitiontable[i].start_head);
        printf(" %4d", partitiontable[i].start_sec);
        printf(" %4d", partitiontable[i].start_cyl);
        printf(" %#4x", partitiontable[i].type);
        printf(" %4d", partitiontable[i].end_head);
        printf(" %4d", partitiontable[i].end_sec);
        printf(" %4d", partitiontable[i].end_cyl);
        printf(" %10d", partitiontable[i].lFirst);
        printf(" %10d\n", partitiontable[i].size);
    }
}

void getSuperBlock(FILE *image, uintptr_t offset, SuperBlock *superblock)
{
    /* Go to superblock */
    if (-1 == fseek(image, offset + 1024, SEEK_SET))
    {
        perror("fseek failed!");
        exit(-1);
    }
    
    /* Read superblock */
    if(fread(superblock, sizeof(SuperBlock), 1, image) != 1)
    {
        perror("superblock read failed!");
        exit(-1);
    }

    /* Validate superblock */
    if (superblock->magic != 0x4D5A)
    {
        printf("Invalid super block\n");
        exit(-1);
    }
}

void printSuperblock(SuperBlock sb)
{
    printf("\nSuperblock Contents:\n");
    printf("  %-10s %10d\n", "ninodes", sb.ninodes);
    printf("  %-10s %10d\n", "i_blocks", sb.i_blocks);
    printf("  %-10s %10d\n", "z_blocks", sb.z_blocks);
    printf("  %-10s %10d\n", "firstdata", sb.firstdata);
    printf("  %-13s %7d", "log_zone_size", sb.log_zone_size);
    printf(" (zone size: %d)\n", sb.blocksize << sb.log_zone_size);
    printf("  %-10s %10u\n", "max_file", sb.max_file);
    printf("  %-10s %#10x\n", "magic", sb.magic);
    printf("  %-10s %10d\n", "zones", sb.zones);
    printf("  %-10s %10d\n", "blocksize", sb.blocksize);
    printf("  %-10s %10d\n", "subversion", sb.subversion);
}

int count = 0;

Dirent getDirent
   (FILE* image, Inode inode, SuperBlock sb, uintptr_t offs, uint32_t index)
{
   uint32_t zoneSize = sb.blocksize << sb.log_zone_size;
   uint32_t direntsInZone = zoneSize / sizeof(Dirent);
   uint32_t ptrsInZone = zoneSize / sizeof(uint32_t);
   uint32_t zoneNumber = index / direntsInZone;
   Dirent dummy;
   Dirent* zone;
   
   if ((zone = malloc(zoneSize)) == NULL)
   {
      perror("malloc zone failed!");
      exit(-1);
   }

   if(1 == getZone(image, ptrsInZone, inode, zoneNumber, zone, sb, offs))
   {
      dummy.inode = 0;
      return dummy;
   }

   dummy = zone[index%direntsInZone];

   free(zone);
   return dummy;
}

uint32_t checkDirent(Dirent dirent, char* token)
{
   char direntName[61];
   memset(direntName, '\0', 61);
   memcpy(direntName, dirent.name, 60);
   if(dirent.inode == 0)
   {
      return 0;
   }
   if(strcmp(direntName, token) == 0)
   {
      return dirent.inode;
   }
   else
   {
      return 0;
   }
}

void getInode(FILE *image, uintptr_t offset, uint32_t index, Inode *inode)
{
    uintptr_t inodeoffset;

    inodeoffset = offset + (index - 1) * (sizeof(Inode));

    /* Go to inode table */
    if (-1 == fseek(image, inodeoffset, SEEK_SET))
    {
        perror("fseek failed!");
        exit(-1);
    }

    /* Read inode */
    if(fread(inode, sizeof(Inode), 1, image) != 1)
    {
        perror("inode read failed!");
        exit(-1);
    }
}

void printInode(Inode inode)
{
    int i;
    time_t atimes = (time_t)inode.atime;
    time_t mtimes = (time_t)inode.mtime;
    time_t ctimes = (time_t)inode.ctime;

    printf("\nFile inode:\n");
    printf("  %-20s %#13x", "unsigned short mode", inode.mode);
    printf("    (");
    printMask(inode);
    printf(")\n");
    printf("  %-20s %13d\n", "unsigned short links", inode.links);
    printf("  %-20s %13d\n", "unsigned short uid", inode.uid);
    printf("  %-20s %13d\n", "unsigned short gid", inode.gid);
    printf("  %-15s %13d\n", "uint32_t  size", inode.size);
    printf("  %-15s %13d --- %s", "uint32_t  atime", inode.atime, 
        ctime(&atimes));
    printf("  %-15s %13d --- %s", "uint32_t  mtime", inode.mtime, 
        ctime(&mtimes));
    printf("  %-15s %13d --- %s", "uint32_t  ctime", inode.ctime, 
        ctime(&ctimes));

    printf("\n  Direct zones:\n");
    for (i = 0; i < DIRECT_ZONES; i++)
    {
        printf("              zone[%d]   =%11d\n", i, inode.zone[i]);
    }
    printf("   uint32_t  indirect   =%11d\n", inode.indirect);
    printf("   uint32_t  double     =%11d\n", inode.two_indirect);
}

uint32_t getZone(FILE* image, uint32_t indirectSize, Inode inode, 
    uint32_t index, Dirent* Zone, SuperBlock superblock, uintptr_t offset)
{
    uint32_t blockIndex = 0;
    uintptr_t addr;
    uintptr_t two_indirect_addr;
    uintptr_t indirect_addr;
    uint32_t* tempZone1;
    uint32_t* tempZone2;
    uint32_t zoneSize;
    uint32_t indirIndex;

    /* Calculate size of zone */
    zoneSize = superblock.blocksize << superblock.log_zone_size;

    /* If zone is in Direct */
    if (index < DIRECT_ZONES)
    {
        /* Grab block num of zone array */
        blockIndex = inode.zone[index];
    }
    /* If zone is in Indirect */
    else if (index < DIRECT_ZONES + indirectSize)
    {
        /* Remove Direct Block offset */
        index -= DIRECT_ZONES;

        /* Check if valid block num */
        if (inode.indirect == 0)
        {
            return 1;
        }

        /* Calculate indirect offset */
        addr = inode.indirect * superblock.blocksize;

        /* Temp Zone used to hold array of block numbers */
        if ((tempZone1 = malloc(zoneSize)) == NULL)
        {
            perror("malloc zone failed!");
            exit(-1);
        }

        /* Go to Indirect table */
        if (-1 == fseek(image, offset + addr, SEEK_SET))
        {
            perror("fseek failed!");
            exit(-1);
        }

        /* Read Indirect table */
        if(fread(tempZone1, zoneSize, 1, image) != 1)
        {
            perror("indirect table failed!");
            exit(-1);
        }

        blockIndex = tempZone1[index];

        free(tempZone1);
    }
    /* If zone is in Two_Indirect */
    else if (index < DIRECT_ZONES + indirectSize + 
        (indirectSize * indirectSize))
    {
        /* Remove Direct and Indirect Zone */
        index -= DIRECT_ZONES + indirectSize;

        /* Check if valid block num */
        if (inode.two_indirect == 0)
        {
            return 1;
        }

        /* Calculate two_indirect address */
        two_indirect_addr = inode.two_indirect * superblock.blocksize;

        if ((tempZone1 = malloc(zoneSize)) == NULL)
        {
            perror("malloc zone failed!");
            exit(-1);
        }

        if ((tempZone2 = malloc(zoneSize)) == NULL)
        {
            perror("malloc zone failed!");
            exit(-1);
        }

        /* Go to Two_Indirect table */
        if (-1 == fseek(image, offset + two_indirect_addr, SEEK_SET))
        {
            perror("fseek failed!");
            exit(-1);
        }

        /* Read Two_Indirect table */
        if(fread(tempZone1, zoneSize, 1, image) != 1)
        {
            perror("two_indirect table failed!");
            exit(-1);
        }

        indirIndex = index / (zoneSize / sizeof(uint32_t));

        /* Check if valid block num */
        if (tempZone1[indirIndex] == 0)
        {
            return 1;
        }

        indirect_addr = tempZone1[indirIndex] * superblock.blocksize;

        /* Go to Indirect table */
        if (-1 == fseek(image, offset + indirect_addr, SEEK_SET))
        {
            perror("fseek failed!");
            exit(-1);
        }

        /* Read Indirect table */
        if(fread(tempZone2, zoneSize, 1, image) != 1)
        {
            perror("indirect table failed!");
            exit(-1);
        }

        blockIndex = tempZone2[index % (zoneSize / sizeof(uint32_t))];

        free(tempZone1);
        free(tempZone2);
    }
    else
    {
        printf("Index out of range!\n");
        exit(-1);
    }

    /* Check if valid block num */
    if (blockIndex == 0)
    {
        return 1;
    }

    addr = blockIndex * zoneSize;

    /* Go to Zone */
    if (-1 == fseek(image, offset + addr, SEEK_SET))
    {
        perror("fseek failed!");
        exit(-1);
    }

    /* Read Zone */
    if(fread(Zone, zoneSize, 1, image) != 1)
    {
        perror("indirect table failed!");
        exit(-1);
    }

    return 0;
}

uint32_t checkZone(char* token, Dirent* Zone, uint32_t numEntries)
{
    int i;
    Dirent dirent;
    char direntName[61];

    for (i = 0; i < numEntries; i++)
    {
        dirent = Zone[i];

        if (dirent.inode == 0)
        {
            continue;
        }

        memset(direntName, '\0', 61);
        memcpy(direntName, dirent.name, 60);

        if (strcmp(direntName, token) == 0)
        {
            return dirent.inode;
        }
    }
    return 0; /* File not found in this zone */
}

/* Returns true if inode points to directory, false if not */
int checkDir(Inode inode)
{
    if(inode.mode & DIR_MASK)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void printMask(Inode inode)
{
    char out[11] = "----------";
    if(inode.mode & DIR_MASK)
        out[0] = 'd';
    if(inode.mode & O_R_MASK)
        out[1] = 'r';
    if(inode.mode & O_W_MASK)
        out[2] = 'w';
    if(inode.mode & O_X_MASK)
        out[3] = 'x';
    if(inode.mode & G_R_MASK)
        out[4] = 'r';
    if(inode.mode & G_W_MASK)
        out[5] = 'w';
    if(inode.mode & G_X_MASK)
        out[6] = 'x';
    if(inode.mode & X_R_MASK)
        out[7] = 'r';
    if(inode.mode & X_W_MASK)
        out[8] = 'w';
    if(inode.mode & X_X_MASK)
        out[9] = 'x';
    printf(out);
}

void lsfile(Inode inode, char* path)
{
    printMask(inode);
    printf("    %d %s\n", inode.size, path);
}

void lsdir(Inode inode, char* path, FILE* image, SuperBlock sb, 
    uintptr_t po, uintptr_t io)
{
    int i;
    char direntName[61];
    uint32_t totalEntries = inode.size / sizeof(Dirent);
    Dirent dirent;
    Inode tempin;
    printf("%s:\n", path);

    for(i = 0; i < totalEntries; i++)
    {
        dirent = getDirent(image, inode, sb, po, i);
        
        if(dirent.inode != 0)
        {
            getInode(image, io, dirent.inode, &tempin);
            memset(direntName, '\0', 61);
            memcpy(direntName, dirent.name, 60);
            printMask(tempin);
            printf("   %d %s\n", tempin.size, direntName);
        }
    }
}

void minls(Inode inode, char* path, FILE* image, SuperBlock sb, 
    uintptr_t po, uintptr_t io)
{
    /* If inode points to file */
    if(inode.mode & FIL_MASK)
    {
        lsfile(inode, path);
    }
    if(inode.mode & DIR_MASK)
    {
        lsdir(inode, path, image, sb, po, io);
    }
}

void minget(Inode inode, FILE* image, SuperBlock sb, 
    uintptr_t po, Options ops)
{
    int i;
    int fd;
    uint32_t zoneSize = sb.blocksize << sb.log_zone_size;
    uint32_t numZones = inode.size / zoneSize + (inode.size % zoneSize != 0);
    uint32_t bytesLeft = inode.size;
    uint32_t ptrsInZone = zoneSize / sizeof(uint32_t);
    uint8_t* zone;
    uint32_t bytesToWrite = 0;

    /* Open output file for writing */
    if(ops.destpath != NULL)
    {
        fd = open(ops.destpath, O_CREAT|O_WRONLY|O_TRUNC);
    }
    else
    {
        fd = 1;
    }
    if(inode.mode & FIL_MASK)
    {
        if ((zone = malloc(zoneSize)) == NULL)
        {
          perror("malloc zone failed!");
          exit(-1);
        }
        
        for(i = 0; i < numZones; i++)
        {
            if(bytesLeft > zoneSize)
            {
                bytesToWrite = zoneSize;
            }
            else
            {
                bytesToWrite = bytesLeft;
            }

            /* Hole zone, write a zone of 0's*/
            if(1 == getZone(image, ptrsInZone, inode, i, 
                (Dirent *)zone, sb, po))
            {
                write(fd, '\0', bytesToWrite);
            }
            else
            {
                write(fd, zone, bytesToWrite);
            }
            bytesLeft -= bytesToWrite;
        }

        free(zone);
        close(fd);
    }
    else
    {
        printf("This is not a File!\n");
        exit(-1);
    }
}
