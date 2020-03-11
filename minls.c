#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include "minlib.h"

int main(int argc, char* argv[])
{
	int i;
	Options options;
    FILE* image = NULL;
    uintptr_t offset = 0;
    Partition partitiontable[4];
    Partition spartitiontable[4];
    Partition partition;
    SuperBlock superblock;
    Inode inode;
    uint32_t currentIndex = 1;
    uint32_t zoneSize = 0;
    Dirent* Zone = NULL;
    Dirent *direntList;
    char copySrcpath[4096];
    char* token = NULL;
    uint32_t totalEntries = 0;
    uint32_t numZones = 0;
    uint32_t totalEntriesinZone = 0;
    uint32_t numZonesIndirect = 0;

    /* Parse command line args */
    parseOpts(argc, argv, &options);

    /* Open input image */
    if((image = fopen(options.imgfile, "r")) == NULL)
    {
        perror("fopen failed!");
        exit(-1);
    }

    /* If partition passed in */ 
    if (options.part != -1)
    {
        getPartitionTable(image, offset, options.vflag, partitiontable);

        /* Partition Table verbose */
		if(options.vflag)
		{
			printf("\nPartition table:\n");
			printPartitionTable(partitiontable);
		}

        /* Grab user requested partition */
        partition = partitiontable[options.part];

        /* Validate partition type is Minix */
        if (partition.type != 0x81)
        {
            printf("Invalid partition!\n");
            exit(-1);
        }

        /* Find start of new partition */
        offset = partition.lFirst * 512;

        /* If subpartition passed in */
        if (options.spart != -1)
        {
            getPartitionTable(image, offset, options.vflag, spartitiontable);

            /* Subpartition Table verbose */
			if(options.vflag)
			{
				printf("\nSubpartition table:\n");
				printPartitionTable(spartitiontable);
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

    /* Grab superblock */
    getSuperBlock(image, offset, &superblock);

    /* Superblock verbose */
    if(options.vflag)
    {
    	printSuperblock(superblock);
    }
  
  	/* Calculate zone size in bytes */
    zoneSize = superblock.blocksize << superblock.log_zone_size;

    printf("zoneSize: %d\n", zoneSize);

    if ((Zone = malloc(zoneSize)) == NULL)
    {
        perror("malloc zone failed!");
        exit(-1);
    }

    /* Calculating total number of dirents in a zone */
    totalEntriesinZone = zoneSize / sizeof(Dirent);

    /* Number of zone numbers in Indirect */
    numZonesIndirect = zoneSize / sizeof(uint32_t);

    printf("totalEntriesinZone: %d\n", totalEntriesinZone);

    /* Allocating for zone (list of dirents) */
    if ((direntList = malloc(zoneSize)) == NULL)
    {
    	perror("malloc dirent list failed!");
        exit(-1);
    }

    /* Calculate inode table offset */
    offset += superblock.blocksize * (2 + superblock.i_blocks + superblock.z_blocks);

    /* Init srcpath copy */
    memset(copySrcpath, '\0', 4096);

    /* If a path was provided */
    if (options.srcpath != NULL)
    {
    	memcpy(copySrcpath, options.srcpath, strlen(options.srcpath));
    }
    /* A path wasn't given. Default to root */
    else
    {
    	copySrcpath[0] = '/';
    }

    getInode(image, offset, currentIndex, &inode);
    printInode(inode);

    /* Start traversing path */
    token = strtok(copySrcpath, "/");
    while(token != NULL)
    {
    	printf("token: %s\n", token);

    	/* Get inode */
    	getInode(image, offset, currentIndex, &inode);

        printf("inode.size: %d\n", inode.size);

    	/* Calculating total number of dirents */
    	totalEntries = inode.size / sizeof(Dirent);

        printf("totalEntries: %d\n", totalEntries);

        /* Calculating total number of zones used */
        numZones = (inode.size / zoneSize) + (inode.size % zoneSize != 0);

        printf("numZones: %d\n", numZones);

        for (i = 0; i < numZones; i++)
        {
            if (1 == getZone(image, numZonesIndirect, inode, i, Zone, superblock))
            {
                totalEntries -= totalEntriesinZone;
                continue;
            }

            if (totalEntries < totalEntriesinZone)
            {
                if ((currentIndex = checkZone(token, Zone, totalEntries)) != 0)
                {
                    break;
                }
            }
            else
            {
                if ((currentIndex = checkZone(token, Zone, totalEntriesinZone)) != 0)
                {
                    break;
                }
                totalEntries -= totalEntriesinZone;
            }
        }

        if (currentIndex == 0)
        {
            printf("Couldn't find file!\n");
            exit(-1);
        }



    	token = strtok(NULL, "/");
    }

    if (options.vflag)
    {
    	printInode(inode);
    }

    /* Close image */
    if((fclose(image)) == -1)
    {
        perror("fclose failed!");
        exit(-1);
    }

    return 0;
}