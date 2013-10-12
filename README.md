ev3dev Repositories
===================

1. Introduction
2. Features
3. Repository Descriptions
4. Installation Guide
5. How You Can Help

1. Introduction
---------------

This is part of a group of 4 repositories that make up the ev3dev Linux distribution for the LEGO MINDSTORMS EV3. Each of the repositories has the same README.md file so you can read this one and not miss anything :-)

The ev3dev distribution is a full Debian 7 (wheezy) Linux distribution running on the 3.3.x kernel that is customized for the LEGO MINDSTORMS EV3 robotic controller.

2. Features
-----------

 Features above and beyond the official LEGO kernel include:

- Support for Atheros, Realtek, and other wifi chipsets so you're not stuck with one specific wifi dongle.
- Support for ssh terminal sessions instead of plain old (insecure) telnet
- Actual user accounts instead of passwordless root access
- Fully upgradeable and customizable install using standard "apt" tools, running on the brick
- NFS and Samba file share capability
- Access to device drivers through user-space proc filesystem
- Built in text editors like vim and nano
- Prebuilt support for programming languages like perl, awk, python, and more
- Support for all host operating systems including Windows, Mac, Linux, Android, even Blackberry

Don't want to give up your official LEGO MINDSTORMS EV3 kernel and rootfs? You don't need to!

Just install ev3dev on any micorSD card (min 1GB suggested, but can you even buy one that small anymore?) and plug it into the microSD slot on the EV3. The uboot loader will look on the card, find the ev3dev kernel and happily boot that instead!

When you want to use the official LEGO tools, just unplug the ev3dev microSD card and restart the brick.

This is still an early beta, so it's not as polished as the official LEGO offering, but it's getting better every week as we add support for more of the native EV3 drivers.

3. Repository Descriptions
--------------------------

ev3dev - Assorted scripts to build the kernel and rootfs, format a microSD card, install the distribution on the card and customize the rootfs. If you're using Linux then download this repo.

ev3dev-rootfs - a copy of all the files you'll need to install on the microSD card. Download this repository regardless of which operating system you have.

ev3dev-kernel - The source for the kernel. You don't need to download this unless you are going to actually build the kernel. It's a lot easier to just get the kernel image from the ev3dev-rootfs repo.

ev3dev-modules - The source for the LEGO MINDSTORMS EV3 drivers. You don't need to download this unless you are going to build the modules, or you want to know a bit more about how the drivers work. The pre-compiled modules are already int he ev3-rootfs repository.

4. Installation Guide
---------------------

The install instructions will help you do a bare-bones install from the github repositories using a Linux box. If you really need Windows or Mac instructions, please read the "How You Can Help" section.


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

