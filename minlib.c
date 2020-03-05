#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

#define DIRECT_ZONES 7

typedef struct Options
{
    int32_t vflag;
    int32_t part;
    int32_t spart;  
    char* imgfile;
    char* srcpath;
    char* destpath;
} Options;

typedef struct Partition
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

typedef struct SuperBlock 
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

typedef struct Inode 
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

typedef struct Dirent
{
    uint32_t inode;
    unsigned char name[60];
} Dirent;

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

uintptr_t getPartition(FILE *stream, uint32_t index, uintptr_t offset)
{
    Partition parray[4];
    uint8_t magic = 0;
    uint8_t magic2 = 0;
    
    /*Check if partition table is valid*/

    if (-1 == fseek(stream, offset + 510, SEEK_SET)) /*Navigate to signature*/
    {
        perror("fseek failed!");
        exit(-1);
    }

    if(fread(&magic, sizeof(uint8_t), 1, stream) != 1) /*Read valid bits*/
    {
        perror("fread failed!");
        exit(-1);
    }
    if(fread(&magic2, sizeof(uint8_t), 1, stream) != 1) /*Read valid bits*/
    {
        perror("fread failed!");
        exit(-1);
    }

    printf("magic 1: %d\n", magic);
    printf("magic 2: %d\n", magic2);

    if(magic != 0x55 || magic2 != 0xAA)
    {
        perror("Invalid partition table");
        exit(-1);
    }

    /*Begin copying partition table*/

    if (-1 == fseek(stream, offset + 0x1BE, SEEK_SET)) /*Navigate to p-table*/
    {
        perror("fseek failed!");
        exit(-1);
    }

    if(fread(&parray, sizeof(Partition), index + 1, stream) != 1) /*Read p-table*/
    {
        perror("fread failed!");
        exit(-1);
    }

    return parray[index].lFirst;
}

int main(int argc, char* argv[])
{
    Options options;
    FILE* image;
    uintptr_t offset = 0;

    parseOpts(argc, argv, &options);

    debugOptions(options);

    if((image = fopen(options.imgfile, "r")) == NULL) /*Read p-table*/
    {
        perror("fopen failed!");
        exit(-1);
    }

    if (options.part != -1)
    {
        offset = getPartition(image, options.part, offset);
    }

    if (options.spart != -1)
    {
        offset = getPartition(image, options.spart, offset);
    }

    printf("%p\n", (void *)offset);

    return 0;
}