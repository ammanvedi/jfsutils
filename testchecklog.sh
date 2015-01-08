#!/bin/sh

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
echo "--> checking disk consistancy"
echo "---"
echo ""
./jfs_fsck checklogfs
echo ""
echo "----------------------------"
echo ""
