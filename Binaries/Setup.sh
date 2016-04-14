#!/bin/sh


chmod -x /etc/init.d/dphys-swapfile
swapoff -a
rm /var/swap


echo -n " nosmp" >> /boot/cmdline.txt

cat /boot/cmdline.txt | tr -d "\n" > /boot/cmdline2.txt 
mv /boot/cmdline2.txt /boot/cmdline.txt



echo Done.



