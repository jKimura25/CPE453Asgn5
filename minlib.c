#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
   parseOpts(argc, argv);
   return 0;
}

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

int parseOpts(int argc, char* argv[])
{
   int vflag = 0;
   char* part = NULL;
   char* spart = NULL;  
   char* imgfile = NULL;
   int c;
   if(argc == 1)
   {
      printHelp();
      exit(0);
   }
   while((c = getopt (argc, argv, "hvp:s:")) != -1)
   {
      switch (c)
      {
      case 'h':
         printHelp();
         exit(0);
      case 'v':
         vflag = 1;
         break;
      case 'p':
         part = optarg;
         break;
      case 's':
         spart = optarg;
         break;
      default:
         printf("The fuck is this shit?: %c\n", optopt);
      }
   }
   if(part != NULL)
   {
      printf("%s\n", part);
   }
   if(spart != NULL)
   {
      printf("%s\n", spart);
   }
   printf("vflag: %d\n", vflag);
   printf("filename: %s\n", argv[optind]); /*Take filename*/
}
