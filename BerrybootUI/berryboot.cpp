/* Berryboot
 *
 * Copyright (c) 2012, Floris Bos
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "berryboot.h"

#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/reboot.h>


BerryBoot::BerryBoot()
{
    if (!file_exists("/mnt"))
        mkdir("/mnt", 0755);
    if (!file_exists("/boot"))
        mkdir("/boot", 0755);
}

bool BerryBoot::file_exists(const string &name)
{
    return access(name.c_str(), F_OK) == 0;
}

string BerryBoot::file_get_contents(const string &name)
{
    char buffer[4096];
    size_t numread;
    string s;

    FILE *f = fopen(name.c_str(), "rb");
    if (f)
    {
        numread = fread(buffer, 1, sizeof(buffer), f);
        if (numread > 0)
        {
            s.append(buffer, numread);
        }
        fclose(f);
    }

    return s;
}

bool BerryBoot::file_put_contents(const string &name, const string &data)
{
    int written = -1;

    FILE *f = fopen(name.c_str(), "wb");
    if (f)
    {
        written = fwrite(data.c_str(),1, data.size(), f);
        fclose(f);
    }

    return (written == (int) data.size() );
}

string BerryBoot::file_get_contents_trim(const string &name)
{
    string d = file_get_contents(name);
    if (!d.empty() && d[d.length()-1] == '\n')
    {
        d = d.substr(0, d.length()-1);
    }

    return d;
}

map<string,string> BerryBoot::listInstalledImages()
{
    struct dirent *ent;
    map<string,string> images;
    DIR *dir = opendir( "/mnt/images");

    while ( dir && (ent = readdir(dir)) != NULL)
    {
        string name = ent->d_name;

        if (name != "." && name != ".." && name != "default")
        {
            images[name] = imageFilenameToFriendlyName(name);
        }
    }
    closedir(dir);

    return images;
}

bool BerryBoot::mountDataPartition(const string &dev)
{
    //if (execute("mount LABEL=berryboot /mnt") != 0)

    string mountoptions = "-o noatime";
    if (getBootOptions().find("fstype=btrfs") != string::npos)
        mountoptions += ",compress=lzo";

    if (execute(string("mount "+mountoptions+" /dev/")+dev+" /mnt") != 0)
        return false;

    if (!file_exists("/mnt/images"))
    {
        if ( execute("umount /mnt") != 0)
        {
            log_error("Error unmounting data partition");
        }
        return false;
    }

    return true;
}

bool BerryBoot::waitForDevice(const string &dev)
{
    for (unsigned int i=0; i<40; i++)
    {
        if (i != 0)
            usleep(250000);

        if (file_exists("/dev/"+dev))
            return true;
    }

    return false;
}

string BerryBoot::getPartitionByLabel(const string &label)
{
    string cmd = "findfs LABEL="+label+" >/tmp/_dev 2>/dev/null";
    string dev;

    if (system(cmd.c_str()) == 0)
    {
        dev = file_get_contents_trim("/tmp/_dev");
        if (dev.size()>5) /* Remove /dev and trailing \n */
            dev = dev.substr(5);
    }
    unlink("/tmp/_dev");

    return dev;
}


bool BerryBoot::mountSystemPartition()
{
    return (execute("mount /dev/mmcblk0p1 /boot") == 0) || (execute("mount /dev/mmcblk0 /boot") == 0);
}

void BerryBoot::startNetworking()
{
    bool eth0available = false;

    for (unsigned int i=0; i<10 && !eth0available; i++)
    {
        if (i != 0)
            usleep(500000);

        eth0available = file_exists("/sys/class/net/eth0");
    }
    if (!eth0available)
    {
        log_error("eth0 does not seem to be available");
        return;
    }

    if ( execute("/sbin/ifup eth0") != 0)
    {
        log_error("Error bringing up network interfaces");
    }
}

bool BerryBoot::networkReady()
{
    /* Once we have a DHCP release /tmp/resolv.conf is created */
    return file_exists("/tmp/resolv.conf");
}

void BerryBoot::umountSystemPartition()
{
    if ( execute("umount /boot") != 0)
    {
        log_error("Error unmounting system partition");
    }
}

int BerryBoot::execute(const string &cmd)
{
    int result = system( string(cmd+" >/dev/null 2>/dev/null").c_str() );

    if (result != 0)
    {
        log_error("Error executing: "+cmd);
    }

    return result;
}

void BerryBoot::log_error(const string &error)
{
    _lastError = error;
    //cout << error.c_str() << endl;
    //sleep(1);
}

string BerryBoot::imageFilenameToFriendlyName(const string &name)
{
    string n;

    /* Replace underscores with spaces */
    for (unsigned int i=0; i < name.size(); i++)
    {
        if (name[i] == '_')
            n += ' ';
        else
            n += name[i];
    }

    /* If name is a full URL, extract filename */
    size_t slashpos = n.rfind('/');
    if (slashpos != std::string::npos)
        n = n.substr(slashpos+1);

    /* Chop .img extension off */
    size_t imgpos = n.rfind(".img");
    if (imgpos != std::string::npos)
        n = n.substr(0, imgpos);

    return n;
}

string BerryBoot::getBootOptions()
{
    return file_get_contents("/proc/cmdline");
}

int BerryBoot::currentMemsplit()
{
    string meminfo = file_get_contents("/proc/meminfo");
    //  MemTotal:        3713768 kB

    string memtotalstr = meminfo.substr(0, meminfo.find(" kB"));
    memtotalstr = memtotalstr.substr(memtotalstr.rfind(" ")+1);
    int memtotal = atoi(memtotalstr.c_str());

    if (memtotal > 230000)
        return 240;
    else if (memtotal > 200000)
        return 224;
    else if (memtotal > 130000)
        return 192;
    else
        return 128;
}

int BerryBoot::imageNeedsMemsplit(const string &name)
{
    /* image file names are like: berryterminal.img224 */

    size_t imgpos = name.rfind(".img");
    if (imgpos != std::string::npos && imgpos+5 < name.size())
    {
        string suffix = name.substr(imgpos+4);
        return atoi(suffix.c_str());
    }
    else
    {
        return 0;
    }
}

const char *BerryBoot::memsplitFilename(int memsplit)
{
    const char *r;

    switch (memsplit)
    {
        case 240:
            r = "/boot/arm240_start.elf";
            break;
        case 224:
            r = "/boot/arm224_start.elf";
            break;
        case 128:
            r = "/boot/arm128_start.elf";
            break;
        default:
            r = "/boot/arm192_start.elf";
    }

    return r;
}

void BerryBoot::startInstaller()
{
    //mountSystemPartition();
    uncompressInstaller();

    if (getBootOptions().find("vncinstall") != string::npos)
    {
        if (system("clear") != 0) { }
        cout << "VNCInstall option specified in cmdline.txt" << endl;
        cout << "Connect with your VNC client to port 5900 to continue" << endl;
        cout << endl;
        if (system("ifconfig | grep inet") != 0) { }

        execute("/tmp/installer -qws -display VNC:0");
    }
    else
        execute("/tmp/installer -qws");
}

void BerryBoot::uncompressInstaller()
{
    if (!file_exists("/tmp/installer"))
    {
        system("gzip -dc /boot/BerrybootInstaller.gz > /tmp/installer");
        chmod("/tmp/installer", 0755);
    }
}

void BerryBoot::reboot()
{
    system("umount -ar");
    sync();
    ::reboot(RB_AUTOBOOT);
}

bool BerryBoot::isRaspberry()
{
    return file_get_contents("/proc/cpuinfo").find("BCM2708") != string::npos;
}

