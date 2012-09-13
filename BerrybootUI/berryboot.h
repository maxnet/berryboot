#ifndef BERRYBOOT_H
#define BERRYBOOT_H

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

#include <list>
#include <string>
#include <map>

using namespace std;


class BerryBoot
{
public:
    BerryBoot();

    /*
     * Mount data partition
     */
    bool mountDataPartition(const string &dev);

    /*
     * Wait for disk device to become available
     */
    bool waitForDevice(const string &dev);

    /*
     * Get disk device by file system label
     */
    string getPartitionByLabel(const string &label = "berryboot");

    /*
     * Mount boot partition
     */
    bool mountSystemPartition();

    /*
     * Unmount boot partition
     */
    void umountSystemPartition();

    /*
     * Enable network interface
     */
    void startNetworking();

    /*
     * Returns true if DHCP lease is obtained
     */
    bool networkReady();

    /*
     * Returns map with installed images
     *
     * key: raw file name of image
     * value: user friendly name of image
     */
    map<string,string> listInstalledImages();

    /*
     * Gets the user friendly name of a file system image
     */
    string imageFilenameToFriendlyName(const string &name);

    /*
     * Return kernel cmdline
     */
    string getBootOptions();

    /*
     * Return current memory split in MB
     */
    int currentMemsplit();

    /*
     * Return the memory split the image prefers
     */
    int imageNeedsMemsplit(const string &name);

    /*
     * Returns start.elf file name for certain memory split
     */
    const char *memsplitFilename(int memsplit);

    /*
     * Returns true if a file exists
     */
    bool file_exists(const string &name);

    /*
     * Returns contents of file
     */
    string file_get_contents(const string &name);

    /*
     * Write string to file
     */
    bool file_put_contents(const string &name, const string &data);

    /*
     * Start operating system installer
     */
    void startInstaller();

    /*
     * Uncompress operating system installer
     */
    void uncompressInstaller();

    /*
     * Reboot system
     */
    void reboot();

protected:
    /*
     * Returns contents of file with beginning and trailing white space removed
     */
    string file_get_contents_trim(const string &name);

    /*
     * Executes a shell command
     */
    int execute(const string &cmd);

    /*
     * Record error
     */
    void log_error(const string &error);

    string _lastError;
};

#endif // BERRYBOOT_H
