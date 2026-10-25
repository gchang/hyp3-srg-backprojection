#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

long long filelen_(char *filename){

  long long buf[20];
  struct stat sb;
  int ierr,i;
  char fname[1024];

  for(i=0;i<1023 && filename[i]!=0 && filename[i]!=32;i++)fname[i]=filename[i];
  fname[i]=0;
  /*  for(i=0; i<20; i++)printf("fname: %d %lld\n",i,fname[i]); */

  /*  null terminate file name */
  for(i=0;i<1024;i++){
    if(fname[i]==32)fname[i]=0; 
    if(fname[i]==0)break;
  }

  ierr = stat(fname, &sb);
  /* for(i=0; i<20; i++)printf("buf: %lld\n",buf[i]);  */
  /*           printf("I-node number:            %ld\n", (long) sb.st_ino);

           printf("Mode:                     %lo (octal)\n",
                   (unsigned long) sb.st_mode);

           printf("Link count:               %ld\n", (long) sb.st_nlink);
           printf("Ownership:                UID=%ld   GID=%ld\n",
                   (long) sb.st_uid, (long) sb.st_gid);

           printf("Preferred I/O block size: %ld bytes\n",
                   (long) sb.st_blksize);
           printf("File size:                %lld bytes\n",
                   (long long) sb.st_size);
           printf("Blocks allocated:         %lld\n",
                   (long long) sb.st_blocks);
  */
  return sb.st_size;
}
