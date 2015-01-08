#!/bin/sh
clear
echo "attempt cleanup"
rm testfs
echo "creating new fs 'testfs'"
./jfs_format testfs
./jfs_mkdir testfs amman
echo "copying README to amman/readme & rdme to amman/testfile"
./jfs_copyin testfs README amman/readme
./jfs_copyin testfs rdme amman/testfile
echo "finished making 'testfs'"
echo "inital contents;"
echo "-------------------------------"
./jfs_ls testfs
echo "-------------------------------"
echo ""
echo "attempt removal of readme from directory"
echo " "
echo " "
echo "===========================MODULE OUTPUT============================"
echo " "
./jfs_rm testfs amman readme
echo " "
echo "===================================================================="
echo " "
echo " "
echo "listing contents after rm operation"
echo "-------------------------------"
./jfs_ls testfs
echo "-------------------------------"
echo "also check disk consistancy"
echo ""
./jfs_fsck testfs
echo "-------------------------------"
echo " "
