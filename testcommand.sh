#!/bin/sh

echo "creating new fs 'testfs'"
./jfs_format testfs
./jfs_mkdir testfs amman
./jfs_copyin testfs README amman/readme
./jfs_copyin testfs rdme amman/testfile
echo "finished making 'testfs'"