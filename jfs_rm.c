#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fs_disk.h"
#include "jfs_common.h"


/*
    Remove a file from any directory
    
*/
void remove_file(jfs_t *jfs, char *pname, char *fname)
{
    int rootinode = find_root_directory(jfs);
    struct inode in;
    int bytes = 0, foundfile = 0;
    int resultdir;
    
    if(strcmp(pname, "/") == 0)
    {
        //the suser is requesting the root directory in which case
        //no further lookup is needed 
        resultdir = rootinode;
    }else{
        char pathbuf[strlen(pname)];
        last_part(pname, &pathbuf);
        resultdir  = findfile_recursive(jfs, pathbuf, rootinode, DT_DIRECTORY);        
    }

    if(resultdir == -1)
    {
      printf("jfs_rm:ERROR --> directory not found\n");
      exit(1);
    }

    get_inode(jfs, resultdir, &in);
    int dir_size = in.size;
    char inspectblock[BLOCKSIZE];
    struct dirent* d;
    //directory consists of single block that stores all entries
    //read the first block in list and iterate through it to discern entries
    jfs_read_block(jfs, inspectblock, in.blockptrs[0]);
    //cast data to directory entry
    d = (struct dirent*)inspectblock;
    //create data block that will hold directory listing data
    //after desired items have been removed
    char editedblock[BLOCKSIZE];
    //keep track of the end of the edited block for subsequent writes
    int endofeditblock = 0;

    while(1)
    {
        //null terminate filename for reference
        char filename[MAX_FILENAME_LEN + 1];
        memcpy(filename, d->name, d->namelen);
        filename[d->namelen] = '\0';

        if (d->file_type == DT_DIRECTORY)
        {
            //retrieve directory data
            struct inode dinode;
            get_inode(jfs, d->inode, &dinode);
            //rm removes files only, directories can be written into edited
            //directory
            memcpy((editedblock + endofeditblock), (inspectblock + bytes), d->entry_len);
            endofeditblock += d->entry_len;
        }

        if (d->file_type == DT_FILE)
        {

            //retieve file inode
            struct inode dirnode;
            get_inode(jfs, d->inode, &dirnode);

            if(strcmp(filename, fname) == 0)
            {
                printf("jfs_checklog:INFO --> Found file\n");
                //filename matches file to remove
                //this entry will not be copied into edited block
                //its held resources must be freed
                foundfile = 1;
                int y;

                for(y = 0; y < INODE_BLOCK_PTRS; y++)
                {

                    //blocks that are used by the file should be freed
                    //to maintain consistancy
                    int blocktofree = dirnode.blockptrs[y];
                    if(blocktofree != 0)
                    {
                        return_block_to_freelist(jfs, blocktofree);
                    }
                }

                //return inode that held file information to freelist
                printf("jfs_checklog:INFO --> Freeing File Resources\n");
                return_inode_to_freelist(jfs, d->inode);

                char inodetoeditbuf[BLOCKSIZE];
                struct inode *editnodepointer;
                //retrieve the block that stores the directory inode
                jfs_read_block(jfs, inodetoeditbuf, inode_to_block(resultdir));

                //get pointer to inode
                editnodepointer = (struct inode*)(inodetoeditbuf + (resultdir % INODES_PER_BLOCK) * INODE_SIZE);

                //length of directory entry has changed due to removal of an entry
                //make sure new size is applied
                editnodepointer->size = (in.size - d->entry_len);
                jfs_write_block(jfs, inodetoeditbuf, inode_to_block(resultdir));


            }else
            {
                //file entry found in the directory listing, but it is not the file to be
                //removed, copy entry to edited block
                memcpy((editedblock + endofeditblock), (inspectblock + bytes), d->entry_len);
                endofeditblock += d->entry_len;
            }
        }


        bytes += d->entry_len;
        d = (struct dirent*)(inspectblock + bytes);

        if (bytes >= dir_size)
        {
            break;
        }

    }


        //completed reading the directory entry
        //edited block contains a copy of original directory
        //entry, without the directory to be removed
        jfs_write_block(jfs, editedblock, in.blockptrs[0]);
        jfs_commit(jfs);

        if(foundfile == 0)
        {
          printf("jfs_rm:ERROR --> File not found\n");
          exit(1);
        }

        
}

void showusage()
{
  fprintf(stderr, "Usage: jfs_rm <volumename> <path> <file to delete>\n");
  exit(1);
}


int main(int argc, char **argv)
{

    
    if(argc < 4)
    {
        showusage();
    }
    
    struct disk_image *di;
    jfs_t *jfs;
    di = mount_disk_image(argv[1]);
    jfs = init_jfs(di);
    remove_file(jfs, argv[2], argv[3]);
    unmount_disk_image(di);
    exit(0);
}
