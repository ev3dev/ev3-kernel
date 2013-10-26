ev3dev Repositories
===================

1. Introduction
2. Features
3. Repository Descriptions
4. Installation Guide
5. How You Can Help


# 1. Introduction

This is part of a group of 4 repositories that make up the ev3dev Linux distribution for the LEGO MINDSTORMS EV3. Each of the repositories has the same README.md file so you can read this one and not miss anything :-)

The ev3dev distribution is a full Debian 7 (wheezy) Linux distribution running on the 3.3.x kernel that I have customized for the LEGO MINDSTORMS EV3 controller.

The boot screen you'll be greeted with is you use the special serial port debugger is:

````
             _____     _
   _____   _|___ /  __| | _____   __
  / _ \ \ / / |_ \ / _` |/ _ \ \ / /
 |  __/\ V / ___) | (_| |  __/\ V / 
  \___| \_/ |____/ \__,_|\___| \_/  

Debian GNU/Linux 7 on LEGO MINDSTORMS EV3!
````

How neat is that?!!!

2. Features
-----------

Features above and beyond the official LEGO kernel include:

- Support for Atheros, Realtek, and other wifi chipsets so you're not stuck with one specific wifi dongle
- Support for ssh terminal sessions instead of plain old (insecure) telnet
- Actual user accounts instead of passwordless root access
- Fully upgradeable and customizable install using standard "apt" tools, running on the brick
- NFS and Samba file share capability
- Access to device drivers through user-space proc filesystem
- Built in text editors like vim and nano
- Prebuilt support for programming languages like perl, gawk, python, and more
- Support for all host operating systems including Windows, Mac, Linux, Android, even Blackberry

Don't want to give up your official LEGO MINDSTORMS EV3 kernel and rootfs? You don't need to!

Just install ev3dev on any microSD card (min 1GB suggested, but can you even buy one that small anymore?) and plug it into the microSD slot on the EV3. The uboot loader will look on the card, find the ev3dev kernel and happily boot that instead!

When you want to use the official LEGO tools, just shutdown the EV3, unplug the ev3dev microSD card and restart the brick.

This is still an early beta, so it's not as polished as the official LEGO offering, but it's getting better every week as we add support for more of the native EV3 drivers.

Also, the unofficail wifi dongle I have tried is the Asus N10, see the section on "How You Can Help" to get support for more dongles.

Finally, the boot time is a terrible 2 minutes! I'll be looking for ways to speed that up, of course.

3. Repository Descriptions
--------------------------

The ev3dev system is kept in 4 repositories, each with a specific purpose. To get started, just grab the +ev3dev+ and +ev3dev-rootfs+ repositories. If you really, really want to hack the kernel and driver modules, then grab the other two as well.

ev3dev - Assorted scripts to build the kernel and rootfs, format a microSD card, install the distribution on the card and customize the rootfs. Download this repository regarless of which operating system you have.

ev3dev-rootfs - a copy of all the files you'll need to install on the microSD card. Download this repository regardless of which operating system you have.

ev3dev-kernel - The source for the kernel. You don't need to download this unless you are going to actually build the kernel. It's a lot easier to just get the kernel image from the ev3dev repo.

ev3dev-modules - The source for the LEGO MINDSTORMS EV3 drivers. You don't need to download this unless you are going to build the modules, or you want to know a bit more about how the drivers work. The pre-compiled modules are already in the ev3-rootfs repository.

4. Installation Guide
---------------------

The install instructions will help you do a bare-bones install from the github repositories using a Linux box. If you really need Windows or Mac instructions, please read the "How You Can Help" section.

I'm going to assume you have git installed and set up on your computer, and that you have some basic knowledge of how to use it. Even if you're a complete git noob, it's still pretty easy to get the two repositories you always need set up.

4.1 Set Up Directories and Get Repositories
-------------------------------------------

I'm going to suggest that you set up your ev3dev environment in your HOME directory for now. If we decide to change things around it's pretty easy to symlink things to get the right layout.

I tend to do a lot of development using nfs mounts, so here's the location going to suggest:

~/nfs/ev3

After we get things going, we can mount this directory on the EV3 - which means the EV3 can get at our scripts more easily, and we can copy stuff to the microSD card without having to unplug the card from the EV3.

Create your new directory, navigate to it and then run the following commands:

````
mkdir -p ~/nfs/ev3
cd       ~/nfs/ev3

git clone git@github.com:mindboards/ev3dev.git
git clone git@github.com:mindboards/ev3dev-rootfs.git
````

Now that you have the script directory and the rootfs directory, it's a simple matter to set up the SD Card and write the new image to it, like this:

````
sudo ./write_sdcard_img
````

4.2 Customizing Your ev3dev-rootfs
----------------------------------

OK, at this point you should be able to mount your SD card on your PC, and if you want to have the EV3 to connect to your wifi network, then you'll need to customize two files. To get the mount points to work you'll need to update the file 10-ev3dev.rules and copy it to your /etc/udev/rules.d directory.

````
# If you go through building the rootfs from scratch then the mount points
# are automatically created, but let's create them manually here anyways...

cd ~/nfs/ev3
mkdir LMS2012
mkdir LMS2012_EXT

# And then mount the rootfs

sudo mount /dev/ev3dev_2 LMS2012_EXT

# Now you can edit files in LMS2012_EXT and they will be updated on the SD Card
# When you're done, remember to unmount the card like this

sudo umount /dev/ev3dev_2
````

The file you want to customize with the SSID and WPA Password of your wifi network is:

````
LMS2012_EXT/etc/network/interfaces

# Here is the block to add to that file:

auto wlan0
iface wlan0 inet dhcp

  wpa-scan-ssid 1
  wpa-ap-scan 1
  wpa-key-mgmt WPA-PSK
  wpa-proto RSN WPA
  wpa-pairwise CCMP TKIP
  wpa-group CCMP TKIP
  wpa-ssid "YourSSIDHere"
  wpa-psk  "YourPassPhraseHere"

# Note that you need to put your actual SSID and Password in :-)
#
# If you don't want your cleartext passphrase in the file, you can
# use Google to change it to the hex code
````

4.3 First Boot
--------------

Now, unmount the card and remove it from your host machine, then put it into the
the EV3 and reboot. It takes about 2 minutes to boot, and it's more fun to 
watch the text go by on the serial console, if you have one.

Depending on your DHCP setup, you'll be able to ssh in to your EV3 like so:

````
# The default passwords are
# 
# User: ev3dev Password: 3v3d3v
# User: root   Password: r00tme

ssh ev3dev@192.168.x.x

# Now you have a complete Debian 7 EV3!
````

4.3 Building The rootfs From Scratch
------------------------------------

If you're really abitious, you can build the rootfs from scratch, all the required scripts are in the ev3dev folder. Here are the exact steps I use on a clean, fresh, minimal Ubuntu or Debian machine. I'll leave it to you to get access to such a machine somehow.

````
# ------------------------------------------------------------------------------
# First we need to get all the additional apps that are not typically on a
# clean Linux machine using:

cd ev3dev

sudo ./apt_get_multistrap

# Then we build the ev3-rootfs

sudo ./build_rootfs

# Then we create the SD card partitions and populate the card - you want
# to review the contents of 10-ev3dev.rules to see how we automagically
# create /dev entries for the SD card . Finally, create the ev3dev.img
# file and gzip it for easy distribution.

sudo ./create_sdcard
sudo ./populate_sdcard
sudo ./read_sdcard_img
````

5. How You Can Help
-------------------

Building this custom Linux distribution is a lot of work, even on a Linux host system. Because I strongly dislike doing tedious tasks more than once, and I'm a terrible typist, I decided to script a lot of the mudane tasks like building and customizing the rootfs image, and mounting/formatting/populating the microSD card.

Naturally the scripts run on pretty much any modern Linux system, but they don't work on Windows or Mac systems yet because ... well because I don't have those systems. OK, I have a Windows system at work and I'm very familiar with Windows scripting, but I can't/won't use work resources for this.

Also, I only have a couple of Realtek WiFi mini-dongles (like the Asus N10) and no other USB devices like sound cards or cameras. Here's where you come in.

If I had a Mac laptop (even an older Intel machine would do) then I could run Windows and Linux in VirtualBox on that Mac. Which means I could develop install scripts for all three major platforms. Also, if you want support for a particular WiFi dongle (or any other USB device) then send me a device and I'll do my best to support it.

So, if you have an older Mac laptop that's not doing anything, or you feel super generous and want to donate something like a 13" MacBook Air to the cause, contact me and we'll work something out. I just cannot justify spending additional money on what is essentially a hobby project.

I've done a lot of the heavy lifting to get the distribution to this point, and I'm happy to take community contributions to the repos. Just send me a pull request and I'll look at your updates.

Cheers, and thanks for looking!

Ralph Hempel
