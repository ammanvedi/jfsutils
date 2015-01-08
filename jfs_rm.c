#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "fs_disk.h"
#include "jfs_common.h"


/*
    remove a file from any directory
*/
int remove_file(jfs_t *jfs, char *pname, char *fname)
{
    int rootinode = find_root_directory(jfs);
    int result  = findfile_recursive(jfs, "amman/readme", rootinode, DT_FILE);
    struct inode in;
    int bytes = 0;
    int resultdir  = findfile_recursive(jfs, pname, rootinode, DT_DIRECTORY);
    get_inode(jfs, resultdir, &in);
    int dir_size = in.size;
    char inspectblock[BLOCKSIZE];
    struct dirent* d;
    //read first block in directory
    jfs_read_block(jfs, inspectblock, in.blockptrs[0]);


    d = (struct dirent*)inspectblock;

    printf("\n\nreading contents of directory %s from block %d\n", pname ,in.blockptrs[0]);
    printf("-- directory SIZE = %d \n\n", in.size);

    char editedblock[BLOCKSIZE];
    int endofeditblock = 0;

    while(1)
    {

        printf("read %d bytes \n\n", bytes);
        char filename[MAX_FILENAME_LEN + 1];
        memcpy(filename, d->name, d->namelen);
        filename[d->namelen] = '\0';

        if (d->file_type == DT_DIRECTORY)
        {
            struct inode dinode;
            get_inode(jfs, d->inode, &dinode);
            printf("directory entry of type DT_DIRECTORY FOUND (ENTRYSIZE is %d bytes)\n", d->entry_len);
            printf("-- %s/%s   SIZE = %d bytes INODE = %d\n", pname, filename, dinode.size, d->inode);
            printf("--- copy bytes %d to %d from buffer to end of new block \n", bytes, (bytes+d->entry_len) -1);
            //memcpy from bytes -> (bytes+d->entry_len) -1) of inspectblock TO
            //       editedblock starting at endofeditedblock
            //set endofeditedblock to new value
            printf("--- copying to end of editedblock at position %d\n", endofeditblock);
            //memcpy
            printf("--- MEMCPY DEST = editedblock + %d SRC = inspectblock + %d Nbytes = %d\n", endofeditblock, bytes, d->entry_len);
            memcpy((editedblock + endofeditblock), (inspectblock + bytes), d->entry_len);
            endofeditblock += d->entry_len;
            printf("--- new end of edited block is %d\n", endofeditblock);
            printf("\n");
        }

        if (d->file_type == DT_FILE)
        {

            struct inode dirnode;
            get_inode(jfs, d->inode, &dirnode);
            printf("directory entry of type DT_FILE FOUND (ENTRYSIZE is %d bytes)\n", d->entry_len);
            printf("-- %s/%s  FILESIZE = %d bytes INODE = %d\n", pname, filename, dirnode.size, d->inode);


            if(strcmp(filename, fname) == 0)
            {

            //printf("the file has inode : %d and can be found in block %d\n", d->inode, inode_to_block(d->inode));
                printf("-- this is the right file\n");
                printf("-- must reduce inode %d SIZE from %d to %d\n", resultdir, in.size, (in.size - d->entry_len));
                printf("--- DO NOT COPY ENTRY TO NEW BLOCK\n");

                printf("-- The directory entry points to a file at inode %d\n", d->inode);
                printf("-- this inode can be returned to the freelist\n");
                printf("-- all blocks pointed to in inode %d can be returned to freelist\n", d->inode);
                printf("-- retrieving inode %d\n", d->inode);

                struct inode filenode;
                get_inode(jfs, d->inode, &filenode);

                int y;

                for(y = 0; y < INODE_BLOCK_PTRS; y++)
                {

                    int blocktofree = filenode.blockptrs[y];
                    if(blocktofree != 0)
                    {
                        printf("--- should free block %d\n", blocktofree);
                        return_block_to_freelist(jfs, blocktofree);
                        printf("--- freed block %d\n", blocktofree);
                    }
                }

                return_inode_to_freelist(jfs, d->inode);
                printf("returned inode %d to freelist\n", d->inode);

                // the file has been found
                // it is time to edit the inode
                // resultdir holds the inode number
                char inodetoeditbuf[BLOCKSIZE];
                struct inode *editnodepointer;
                jfs_read_block(jfs, inodetoeditbuf, inode_to_block(resultdir));
                printf("--- the inode %d resides in block %d\n", resultdir, inode_to_block(resultdir));
                editnodepointer = (struct inode*)(inodetoeditbuf + (resultdir % INODES_PER_BLOCK) * INODE_SIZE);

                printf("--- about to edit an inode of SIZE = %d\n", editnodepointer->size);
                editnodepointer->size = (in.size - d->entry_len);
                printf("--- new SIZE = %d\n", editnodepointer->size);
                printf("--- MUST WRITE!!!! 'inodetoeditbuf' back to block %d\n", inode_to_block(resultdir));
                jfs_write_block(jfs, inodetoeditbuf, inode_to_block(resultdir));
                printf("performed a JFS_WRITE_BLOCK to block %d\n", inode_to_block(resultdir));
                printf("--- edited directory inode keeps entries in block %d\n", editnodepointer->blockptrs[0]);
                printf("\n");

                printf("  MUST REMOVE ENTRY INFORMATION FROM BLOCK %d\n", editnodepointer->blockptrs[0]);
                printf("--- edited directory inode keeps entries in block %d\n", editnodepointer->blockptrs[0]);
                printf("    -- must remove bytes %d to %d from buffer block and write back to block %d \n", bytes, bytes+d->entry_len,  in.blockptrs[0]);
                printf("\n");



            }else
            {

                printf("--- copy bytes %d to %d from buffer to end of new block \n", bytes, (bytes+d->entry_len) -1);
                printf("--- copying to end of editedblock at position %d\n", endofeditblock);
                printf("--- MEMCPY DEST = editedblock + %d SRC = inspectblock + %d Nbytes = %d\n", endofeditblock, bytes, d->entry_len);
                memcpy((editedblock + endofeditblock), (inspectblock + bytes), d->entry_len);
                endofeditblock += d->entry_len;
                printf("--- new end of edited block is %d\n", endofeditblock);
                printf("\n");

            }
        }

        bytes += d->entry_len;
        d = (struct dirent*)(inspectblock + bytes);

        if (bytes >= dir_size)
        {
            break;
        }

    }

        printf("- FINISHED INSPECTING BLOCK, MUST WRITE!!! editedblock back to block position %d\n", in.blockptrs[0]);
        jfs_write_block(jfs, editedblock, in.blockptrs[0]);
        printf("performed a JFS_WRITE_BLOCK to block %d\n", in.blockptrs[0]);
        printf("all changes made, changes must be committed to filesystem, JFS COMMIT!!\n");
        jfs_commit(jfs);
        return 0;
}

int main(int argc, char **argv)
{
    struct disk_image *di;
    jfs_t *jfs;
    di = mount_disk_image(argv[1]);
    jfs = init_jfs(di);
    remove_file(jfs, argv[2], argv[3]);
    unmount_disk_image(di);
    exit(0);
}
