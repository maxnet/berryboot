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


#include "mainwindow.h"
#include <dialog.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define runonce_file  "/mnt/data/runonce"
#define default_file  "/mnt/data/default"

MainWindow::MainWindow()
{
    init_dialog(stdin, stdout);
    dialog_vars.backtitle = (char *) "BerryBoot v1.2";
    dialog_vars.item_help = false;
}

MainWindow::~MainWindow()
{
    end_dialog();
}

void MainWindow::reinitScreen()
{
    /* FIXME: there must be a better way to do this */
    end_dialog();
    erase();
    init_dialog(stdin, stdout);
    dlg_put_backtitle();
}

void MainWindow::exec()
{
    bool success   = false;
    size_t pos;
    string datadev, options = b.getBootOptions();

    if ((pos = options.find("datadev=")) != string::npos)
    {
        datadev = options.substr(pos+8);
        if ((pos = datadev.find(" ")) != string::npos || (pos = datadev.find("\n")) != string::npos)
            datadev = datadev.substr(0, pos);

        if (datadev == "iscsi")
        {
            startISCSI();
        }
    }
    else
    {
        initializeA10();
        startInstaller();
        return;
    }

    reinitScreen();
    progress("Mounting data partition...", "Waiting for storage... "+datadev);

    /* Wait 10 seconds for named data partition */
    b.waitForDevice(datadev);
    success = b.mountDataPartition(datadev);
    if (!success)
    {
        progress("Searching other partitions...", "Data is not at "+datadev);
        datadev.clear();

        /* Search 4 times */
        for (unsigned int i=0; i<4 && datadev.empty(); i++)
        {
            if (i != 0)
                sleep(1);

            datadev = b.getPartitionByLabel();
        }

        if (!datadev.empty())
        {
            progress("Mounting data partition...", "Found other berryboot partition to mount... "+datadev);
            success = b.mountDataPartition(datadev);
        }
    }
    initializeA10();
    reinitScreen();

    if (!success)
    {
        //dialog_msgbox("No data found...", "No existing data found, performing a new installation",0,0,1);
        dialog_msgbox("No data found...", "Cannot find my data partition :-(",5,50,1);
        return;
    }

    if (b.file_exists(runonce_file))
    {
        progress("Removing runonce file");
        string runonce = b.file_get_contents(runonce_file);
        unlink(runonce_file);
        sync();
        std::cerr << runonce;
        return;
    }

    map<string,string> images = b.listInstalledImages();

    /* Default selection */
    if (!images.empty() && options.find("nobootmenutimeout") == string::npos )
    {
        string def;

        if (b.file_exists(default_file))
            def = b.file_get_contents(default_file);
        else
            def = images.begin()->first;

        if (b.file_exists("/mnt/images/"+def))
        {
            time_t t1 = time(NULL);
            dialog_pause("", "Press <enter> if you wish to access the boot menu...",9,50,2);
            /* return code from dialog_pause is not very useful (returns both OK in case of timeout and when key is pressed)
             * as a workaround just look at the time elapsed */
            if (time(NULL)-t1 >= 3)
            {
                bootImage(def);
                return;
            }
        }
    }

    /* Boot menu */
    const char **menulist = new const char *[(images.size()+1)*2];
    unsigned int nr = 0;

    for (map<string,string>::iterator iter = images.begin(); iter != images.end(); iter++)
    {
        menulist[nr++] = iter->second.c_str();
        menulist[nr++] = "";
    }
    menulist[nr++] = "Operating system installer";
    menulist[nr++] = "";
    dialog_menu("", "Select operating system to boot:", 15, 50, 10, images.size()+1, (char **) menulist);

    if (dialog_vars.input_result && dialog_vars.input_result[0])
    {
        if (strcmp(dialog_vars.input_result, "Operating system installer") == 0)
        {
            startInstaller();
        }
        else
        {
            /* Find image name by friendly name */
            string friendlyname = dialog_vars.input_result;
            for (map<string,string>::iterator iter = images.begin(); iter != images.end(); iter++)
            {
                if (iter->second == friendlyname)
                {
                    bootImage(iter->first);
                    break;
                }
            }
        }
    }

    delete menulist;
}

void MainWindow::bootImage(string name)
{
    int currentmemsplit = b.currentMemsplit();
    int needsmemsplit   = b.imageNeedsMemsplit(name);

    if (b.isRaspberry() && needsmemsplit && needsmemsplit != currentmemsplit)
    {
        mountSystemPartition();
        progress("Changing memsplit and rebooting...");
        //rename("/boot/start.elf", b.memsplitFilename(currentmemsplit));
        //rename(b.memsplitFilename(needsmemsplit), "/boot/start.elf");

        /* Edit config.txt */
        string line, newconfig, oldconfig = b.file_get_contents("/boot/config.txt");
        istringstream is(oldconfig);
        while (!is.eof())
        {
            std::getline(is, line);
            if (line.compare(0,8,"gpu_mem=") != 0) /* Copy all old lines except the gpu_mem one */
                newconfig += line + "\n";
        }
        newconfig += b.memsplitParameter(needsmemsplit);
        b.file_put_contents("/boot/config.txt", newconfig);

        b.umountSystemPartition();
        b.file_put_contents(runonce_file, name);
        system("umount /mnt");
        sync(); sleep(1);
        b.reboot();
    }
    else
    {
        std::cerr << name;
    }
}

void MainWindow::progress(string title, string msg)
{
    if (msg.empty())
    {
        msg = title;
        title = "Status";
    }
    reinitScreen();
    dialog_msgbox(title.c_str(), msg.c_str(),3,50,0);
}

void MainWindow::startInstaller()
{
    mountSystemPartition();
    progress("Enabling network interface");
    b.uncompressInstaller();
    b.startNetworking();
    progress("Starting installer");
    b.startInstaller();
}

void MainWindow::mountSystemPartition()
{
    bool success = false;

    reinitScreen();
    progress("Mounting system partition...", "Waiting for storage...");

    /* Attempt mounting the data partition 10 times, at 0.5 second intervals */
    for (unsigned int i=0; i<10 && !success; i++)
    {
        if (i != 0)
            usleep(500000);

        success = b.mountSystemPartition();
    }

    if (!success)
    {
        dialog_msgbox("Mount error", "Error mounting system partition.",5,50,1);
    }
}

void MainWindow::startISCSI()
{
    mountSystemPartition();
    progress("Loading iSCSI drivers");
    system("gzip -dc /boot/shared.tgz | tar x -C /");
    system("modprobe iscsi_tcp");
    progress("Enabling network interface");
    b.startNetworking();
    progress("Connecting to iSCSI SAN");
    if (system("sh /boot/iscsi.sh 2>/dev/null") != 0)
    {
        dialog_msgbox("iSCSI error", "Error connecting to server",5,50,1);
    }
    system("ln -s /dev/sda1 /dev/iscsi");
    b.umountSystemPartition();
}

void MainWindow::loadModule(string name)
{
    progress("Loading module: "+name);
    string cmd = "modprobe "+name;
    system(cmd.c_str());
}

void MainWindow::initializeA10()
{
    string cpuinfo = b.file_get_contents("/proc/cpuinfo");

    if (cpuinfo.find("sun4i") == string::npos && cpuinfo.find("sun5i") == string::npos)
        return; /* Not an Allwinner A10/A13, return */

    if (b.file_exists("/mnt/shared/lib/modules"))
    {
        /* Use shared modules from disk */
        symlink("/mnt/shared/lib/modules", "/lib/modules");
    }
    else
    {
        /* Not yet installed, uncompress shared.tgz from boot partition into ramfs */
        mountSystemPartition();
        progress("Uncompressing drivers");
        system("gzip -dc /boot/shared.tgz | tar x -C /");
        b.umountSystemPartition();
    }

    /* Some necessary drivers are not compiled into the kernel
       load them as module
       FIXME: driver should be loaded dynamically
     */

    // Video
    //loadModule("lcd");
    //loadModule("hdmi");
    //loadModule("disp");
    loadModule("ump");
    loadModule("mali");
    loadModule("mali_drm");
    // Wifi
    loadModule("8192cu");
    // Popular external USB Ethernet device
    loadModule("qf9700");
}
