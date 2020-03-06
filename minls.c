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
    SuperBlock superblock;

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

    getSuperBlock(image, &superblock, offset);

    printf("%p\n", (void *)offset);

    return 0;
}