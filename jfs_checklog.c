#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fs_disk.h"
#include "jfs_common.h"

/*
    remove a file from any directory
*/
int check_log(jfs_t *jfs)
{
  struct inode lognode;
  int rootinode = find_root_directory(jfs);
  //find the inode number of .log
  int loginodenum  = findfile_recursive(jfs, ".log", rootinode, DT_FILE);
  //retrieve the inode corresponding to the .log file
  get_inode(jfs, loginodenum, &lognode);
  int blockcount;
  char tmplog[BLOCKSIZE];
  struct commit_block* cb;

  int datablockcount = 0;
  int blockstorestoreto = 0;
  int foundcommitat = -1;

  //iterate through each block of the logfile
  for(blockcount = 0; blockcount < INODE_BLOCK_PTRS; blockcount++)
  {
    //read block from logfile
    jfs_read_block(jfs, &tmplog, lognode.blockptrs[blockcount]);
    //try and cast memory to commit block structure
    cb = (struct commit_block*)tmplog;
    //if magic number can be read from data we can be sure this is a
    //commit block
    if((cb->magicnum == 0x89abcdef) && (cb->uncommitted == 1))
    {
      printf("jfs_checklog:INFO --> found commit block with uncommitted changes, will attempt restore \n");
      //discovered the commit block
      //save the index position in order to reference the block number later
      foundcommitat=blockcount;
      int c;
      int sanity=0;
      //count number of blocks that will need to be written
      //ommitting blocknums of -1
      for(c = 0; c < INODE_BLOCK_PTRS; c++)
      {
        if(cb->blocknums[c] != -1)
        {
          blockstorestoreto++;
          sanity+=cb->blocknums[c];
        }

      }

      if((sanity == cb->sum) )
      {
        //checksum is valid
        printf("jfs_checklog:INFO --> checksum from commit_block is valid \n");
        if(blockstorestoreto == datablockcount)
        {
          //number of data blocks discovered in logfile matches
          //number of destination blocks in commit_block
          int l;
          int restorepos=0;
          printf("jfs_checklog:INFO --> writing uncommitted changes to disk\n");
          for(l=(foundcommitat-datablockcount); l < datablockcount; l++)
          {
            char datatorestore[BLOCKSIZE];
            jfs_read_block(jfs, &datatorestore, lognode.blockptrs[l]);
            // a jfs write would cause more data to be written to the log
            //fs write is what jfscommit uses to write to disk and will prevent
            //writing again to logfile
            write_block(jfs->d_img, datatorestore, cb->blocknums[restorepos]);
            restorepos++;

          }
          //finished restoring data
          //commit block is found at position " foundcommitat " in logfile
          //clear the lognode
            int i;
          for (i = 0; i < INODE_BLOCK_PTRS; i++)
              cb->blocknums[i] = -1;
	       cb->sum = 0;
	       cb->uncommitted = 0;
	       write_block(jfs->d_img, tmplog,lognode.blockptrs[foundcommitat]);
          break;
        }else
        {
          //commit_block and number of data blocks discovered do not
          //agree
          printf("jfs_checklog:ERROR --> commit block and discovered blocks do not match\n");
          exit(1);
        }

      }else
      {
        //checksum in invalid
        printf("jfs_checklog:ERROR --> checksum is invalid\n");
        exit(1);
      }

    }else
    {
      //keep a count of non commit blocks encountered before commit block
      datablockcount++;
    }

  }

  if(foundcommitat == -1)
  {
    //no commit block was found in the logfile
    printf("jfs_checklog:ERROR --> no commit block was found in the logfile, cannot repair writes\n");
    exit(1);
  }

return 0;
}

int main(int argc, char **argv)
{
  struct disk_image *di;
  jfs_t *jfs;
  di = mount_disk_image(argv[1]);
  jfs = init_jfs(di);
  check_log(jfs);
  unmount_disk_image(di);
  exit(0);
}
