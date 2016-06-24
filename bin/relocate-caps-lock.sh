#!/bin/sh -
sudo sed -i 's/XKBOPTIONS=""/XKBOPTIONS="ctrl:nocaps"/' /etc/default/keyboard
sudo dpkg-reconfigure keyboard-configuration 

