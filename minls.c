#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "minlib.h"

int main(int argc, char* argv[])
{
	Options options;
    FILE* image;
    uintptr_t offset = 0;
    uint8_t magic[2];
    Partition partitiontable[4];
    Partition spartitiontable[4];
    Partition partition;
    SuperBlock superblock;
    Inode* inodes;

    /* Parse command line args */
    parseOpts(argc, argv, &options);

    if((image = fopen(options.imgfile, "r")) == NULL) /*Read p-table*/
    {
        perror("fopen failed!");
        exit(-1);
    }

    /* If partition passed in */ 
    if (options.part != -1)
    {

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
        if(fread(&partitiontable, sizeof(Partition), 4, image) != 4)
        {
            perror("partition read failed!");
            exit(-1);
        }

        /* Grab user requested partition */
        partition = partitiontable[options.part];

        /* Validate partition type is Minix */
        if (partition.type != 0x81)
        {
            printf("Invalid partition!\n");
            exit(-1);
        }

        /* Partition Table verbose */
		if(options.vflag)
		{
			printf("\nPartition table:\n");
			printPartTable(partitiontable);
		}

        /* Find start of new partition */
        offset = partition.lFirst * 512;

        /* If subpartition passed in */
        if (options.spart != -1)
        {
            /* Go to subpartition table signature */
            if (-1 == fseek(image, offset + 510, SEEK_SET))
            {
                perror("subpartition seek failed!");
                exit(-1);
            }

            /* Read subpartition table signature */
            if(fread(magic, sizeof(uint8_t), 2, image) != 2)
            {
                perror("valid read failed!");
                exit(-1);
            }

            /* Validate subpartition signature */
            if(magic[0] != 0x55 || magic[1] != 0xAA)
            {
                printf("Invalid subpartition table!\n");
                exit(-1);
            }

            /* Go to subpartition table */
            if (-1 == fseek(image, offset + 0x1BE, SEEK_SET))
            {
                perror("subpartition fseek failed!");
                exit(-1);
            }

            /* Read subpartition table */
            if(fread(&spartitiontable, sizeof(Partition), 4, image) != 4)
            {
                perror("subpartition read failed!");
                exit(-1);
            }

            /* Subpartition Table verbose */
			if(options.vflag)
			{
				printf("\nSubpartition table:\n");
				printPartTable(spartitiontable);
			}

            /* Grab user requested subpartition */
            partition = spartitiontable[options.spart];

            /* Validate subpartition type is Minix */
            if (partition.type != 0x81)
            {
                printf("Invalid partition!\n");
                exit(-1);
            }

            /* Find start of new subpartition */
            offset = partition.lFirst * 512;
        }
    }

    /* Got to superblock */
    if (-1 == fseek(image, offset + 1024, SEEK_SET))
    {
        perror("fseek failed!");
        exit(-1);
    }
    
    /* Read superblock */
    if(fread(&superblock, sizeof(SuperBlock), 1, image) != 1)
    {
        perror("superblock read failed!");
        exit(-1);
    }

    /* Validate superblock */
    if (superblock.magic != 0x4D5A)
    {
        printf("Invalid super block\n");
        exit(-1);
    }

    /* Superblock verbose */
    if(options.vflag)
    {
    	printSuperblock(superblock);
    }

    /* Calculate inode table offset */
    offset += superblock.blocksize * (2 + superblock.i_blocks + superblock.z_blocks);

    /* Go to inode table */
    if (-1 == fseek(image, offset, SEEK_SET))
    {
        perror("fseek failed!");
        exit(-1);
    }

    /* Allocating for inode table */
    if ((inodes = malloc(sizeof(uint64_t) * superblock.ninodes)) == NULL)
    {
    	perror("malloc inode table failed!");
        exit(-1);
    }

    /* Read inode table */
    if(fread(inodes, sizeof(uint64_t), superblock.ninodes, image) != superblock.ninodes)
    {
        perror("inode table read failed!");
        exit(-1);
    } 

    /* Close image */
    if((fclose(image)) == -1)
    {
        perror("fclose failed!");
        exit(-1);
    }

    return 0;
}