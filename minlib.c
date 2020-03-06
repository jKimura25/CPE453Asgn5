#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "minlib.h"

void printHelp()
{
    printf("usage: minls [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
    printf("Options:\n");
    printf("-p part    --- select partition for ");
    printf("filesystem (default: none)\n");
    printf("-s sub     --- select subpartition for ");
    printf("filesystem (default: none)\n");
    printf("-h help    --- print usage information and exit\n");
    printf("-v verbose --- increase verbosity level\n");
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
                exit(0);
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

void debugOptions(Options options)
{
    printf("vflag: %d\n", options.vflag);
    printf("part: %d\n", options.part);
    printf("spart: %d\n", options.spart);
    printf("imgfile: %s\n", options.imgfile);
    printf("srcpath: %s\n", options.srcpath);
    printf("destpath: %s\n", options.destpath);
}

void getSuperBlock(FILE *stream, SuperBlock *superblock, uintptr_t offset)
{
    if (-1 == fseek(stream, offset + 1024, SEEK_SET)) /*Navigate to SuperBlock*/
    {
        perror("fseek failed!");
        exit(-1);
    }
    
    if(fread(superblock, sizeof(SuperBlock), 1, stream) != 1) /*Read superblock*/
    {
        perror("superblock read failed!");
        exit(-1);
    }

    printf("superblock ninodes: %d\n", superblock->ninodes);
    printf("superblock magic: %d\n", superblock->magic);

    if (superblock->magic != 0x4D5A)
    {
        printf("Invalid super block\n");
        exit(-1);
    }
}

uintptr_t getPartition(FILE *stream, uint32_t index, uintptr_t offset)
{
    Partition parray[4];
    uint8_t magic = 0;
    uint8_t magic2 = 0;
    uintptr_t newOffset = 0;
    
    printf("partition offset: %p\n", (void *)offset);

    /*Check if partition table is valid*/

    if (-1 == fseek(stream, offset + 510, SEEK_SET)) /*Navigate to signature*/
    {
        perror("partition seek failed!");
        exit(-1);
    }

    if(fread(&magic, sizeof(uint8_t), 1, stream) != 1) /*Read valid bits*/
    {
        perror("valid read failed!");
        exit(-1);
    }
    if(fread(&magic2, sizeof(uint8_t), 1, stream) != 1) /*Read valid bits*/
    {
        perror("valid read failed!");
        exit(-1);
    }

    printf("partition magic 1: %d\n", magic);
    printf("partition magic 2: %d\n", magic2);

    if(magic != 0x55 || magic2 != 0xAA)
    {
        printf("Invalid partition table\n");
        exit(-1);
    }

    /*Begin copying partition table*/

    if (-1 == fseek(stream, offset + 0x1BE, SEEK_SET)) /*Navigate to p-table*/
    {
        perror("fseek failed!");
        exit(-1);
    }

    if(fread(&parray, sizeof(Partition), 4, stream) != 1) /*Read table*/
    {
        perror("partition read failed!");
        exit(-1);
    }

    newOffset = parray[index].lFirst * 512;

    return newOffset ;
}