


ssh-keygen -q -f "/StevesHome/.ssh/known_hosts" -R 10.42.0.27
ssh-copy-id pi@10.42.0.27

scp -r ~/BlockWorks/Alloy/Binaries pi@10.42.0.27:~/

ssh pi@10.42.0.27 "sudo ~/Binaries/Setup.sh"


