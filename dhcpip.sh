#! /bin/bash
sudo cp /etc/network/interfaces_dhcp /etc/network/interfaces
sudo ifdown eth0
sudo ifup eth0
