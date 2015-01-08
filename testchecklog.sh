#!/bin/sh
clear
echo "----------------------------"
echo "-- >setting crash envar"
CRASH_AFTER=13
export CRASH_AFTER
echo $CRASH_AFTER
echo "--> attempting cleanup"
rm checklogfs
echo "--> creating checklogfs"
./jfs_format checklogfs
echo "--> making checklogfs/amman"
./jfs_mkdir checklogfs amman
echo "--> copying README into amman/readme"
./jfs_copyin checklogfs README amman/readme
echo "-->initial consistancy check"
echo "-----------------------------"
echo ""
./jfs_fsck checklogfs
echo ""
echo "attempt fix"
echo " "
echo " "
echo "===========================MODULE OUTPUT============================"
echo " "
./jfs_checklog checklogfs
echo " "
echo "===================================================================="
echo " "
echo " "
echo "test consistancy"
echo "----------------------------"
echo " "
./jfs_fsck checklogfs
echo "----------------------------"
echo " "
