sudo dd if=/dev/zero of=mydisk bs=1024 count=1440
sudo mke2fs -b 1024 mydisk 1440
cc main.c
clear
./a.out mydisk