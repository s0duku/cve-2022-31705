make all && \
sudo insmod ehci.ko && \
sleep 0.5 && \
sudo rmmod ehci.ko && \
sudo dmesg | tail