#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fs_disk.h"
#include "jfs_common.h"

/*
    remove a file from any directory
*/
int check_log(jfs_t *jfs, struct disk_image *dimg)
{
  struct inode lognode;
  int rootinode = find_root_directory(jfs);
  int loginodenum  = findfile_recursive(jfs, ".log", rootinode, DT_FILE);

  get_inode(jfs, loginodenum, &lognode);

  int blockcount;

  //each block is 512 bytes (512 characters)
  //logfile is stored in 14 blocks
  //can read entire log into char[512*INODE_BLOCK_PTRS]

  char logdata[(BLOCKSIZE*INODE_BLOCK_PTRS)+1];
  char tmplog[BLOCKSIZE];
  printf("created logfile buffer of size %d\n", (BLOCKSIZE*INODE_BLOCK_PTRS)+1);
  struct commit_block* cb;

  int datablockcount = 0;
  int blockstorestoreto=0;
  int foundcommitat =0;

  for(blockcount = 0; blockcount < INODE_BLOCK_PTRS; blockcount++)
  {
    printf("\n\nlogfile part resides in block %d\n", lognode.blockptrs[blockcount]);
    printf("reading block of logfile into buffer with an offset from buffer head of %d\n",blockcount*512);
    printf("real memory location of %d\n", ( (logdata + blockcount*512) ));
    //jfs_read_block(jfs, ( (logdata + blockcount*512) ), lognode.blockptrs[blockcount]);
    jfs_read_block(jfs, &tmplog, lognode.blockptrs[blockcount]);
    //try and find a commit block
    cb = (struct commit_block*)tmplog;
    printf("attempted cast to block to commit block, will check for magic number\n");
    if(cb->magicnum == 0x89abcdef)
    {
      printf("FOUND THE COMMIT BLOCK at POSITION %d, here is its data \n", blockcount);
      foundcommitat=blockcount;
      printf("--> magicnum is : %d\n", cb->magicnum);
      printf("--> uncommitted val is : %d\n", cb->uncommitted);
      int c;
      printf("--> preceeding buffered blocks should be committed to memory at\n");
      int sanity=0;
      for(c = 0; c < INODE_BLOCK_PTRS; c++)
      {
        if(cb->blocknums[c] != -1)
        {
          blockstorestoreto++;
          printf("----> block %d\n", cb->blocknums[c]);
          sanity+=cb->blocknums[c];
        }

      }
      printf("--> the checksum of the blocks is %d\n", cb->sum);
      if(sanity == cb->sum)
      {
        printf("--> the checksum is valid\n");
      }else
      {
        printf("the checksum is invalid");
      }
      printf("in order to restore previous data will need to iterate through log from %d to %d\n", (blockcount-datablockcount),blockcount);
      printf("counted %d data blocks, commit block has %d destination blocks\n", datablockcount, blockstorestoreto);

      printf("\n");
      int l;
      int restorepos=0;
      for(l=(foundcommitat-datablockcount); l < datablockcount; l++)
      {
        char datatorestore[BLOCKSIZE];
        jfs_read_block(jfs, &datatorestore, lognode.blockptrs[l]);
        printf("data contained in log block %d will be restored to disk block %d\n", lognode.blockptrs[l], cb->blocknums[restorepos]);
        printf("\n--------------data in block (to restore)----------------\n");
        printf("%s\n", datatorestore);
        printf("--------------end data in block----------------\n\n");
        // a jfs write would cause more data to be written to the log
        //fs write is what jfscommit uses to write to disk and will prevent
        //writing again to logfile
        //use write_block from fs_disk
        write_block(dimg, datatorestore, cb->blocknums[restorepos]);
        printf("FS WRITE PERFORMED\n");
        restorepos++;

      }

    

    }else
    {
      datablockcount++;
      printf("THIS IS A DATA BLOCK :: #%d\n", datablockcount);
      //printf("\n--------------data in block----------------\n");
      //printf("attempting to print partial log \n%s\n\n\n\n", tmplog);
      //sprintf("--------------end data in block----------------\n\n");
    }

  }

  //null terminate string
  logdata[(blockcount*512)+1] = '\0';
  printf("null terminating logfile at position %d\n", (blockcount*512)+1);
  printf("printing the contents of the log file::\n");
  printf("\n");
  printf("%s\n", logdata);

}

int main(int argc, char **argv)
{
  struct disk_image *di;
  jfs_t *jfs;
  di = mount_disk_image(argv[1]);
  jfs = init_jfs(di);
  check_log(jfs, di);
  unmount_disk_image(di);
  exit(0);
}
