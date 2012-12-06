/* Berryboot -- drive format and partitioning thread
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


#include "driveformatthread.h"
#include <unistd.h>
#include <QFile>
#include <QDir>

DriveFormatThread::DriveFormatThread(const QString &drive, const QString &filesystem, Installer *i, QObject *parent, const QString &bootdev, bool initializedata) :
    QThread(parent), _dev(drive), _bootdev(bootdev), _fs(filesystem), _iscsi(false), _initializedata(initializedata), _i(i)
{
    if (_dev == "iscsi")
    {
        _iscsi = true;
        _dev = iscsiDevice();
        _datadev = _dev;
    }
    else if (_dev.startsWith("sd") || _dev.startsWith("hd"))
        _datadev = _dev;
    else
        _datadev = _dev+"p"; // mmcblk0p1 instead of mmcblk01

    _reformatBoot = (_dev == "mmcblk0" || _bootdev != "mmcblk0p1");

    if (_reformatBoot)
        _datadev += "2";
    else
        _datadev += "1";
}

void DriveFormatThread::run()
{
    if (_reformatBoot)
    {
        emit statusUpdate(tr("Saving boot files to memory"));

        if (_i->sizeofBootFilesInKB() > 63000)
        {
            emit error(tr("SD card contains extra files that do not belong to Berryboot. Please copy them to another disk and delete them from card."));
            return;
        }

        if (!_i->saveBootFiles() )
        {
            emit error(tr("Error saving boot files to memory. SD card may be damaged."));
            return;
        }
        _i->umountSystemPartition();
    }

    emit statusUpdate(tr("Zeroing partition table"));
    if (!zeroMbr())
    {
        emit error(tr("Error zero'ing MBR"));
        return;
    }

    emit statusUpdate(tr("Creating partitions"));

    if (!partitionDrive())
    {
        emit error(tr("Error partitioning"));
        return;
    }

    if (_reformatBoot)
    {
        /* A10 devices need to have u-boot written to the spare space before the first partition */
        if (QFile::exists("/tmp/boot/u-boot.bin") && QFile::exists("/tmp/boot/sunxi-spl.bin"))
        {
            emit statusUpdate(tr("Installing u-boot SPL"));
            if (!installUbootSPL())
            {
                emit error(tr("Error writing u-boot to disk"));
                return;
            }
        }

        emit statusUpdate(tr("Formatting boot partition (fat)"));
        if (!formatBootPartition())
        {
            emit error(tr("Error formatting boot partition (vfat)"));
            return;
        }

        if (_initializedata)
        {
            emit statusUpdate(tr("Copying boot files to storage"));
            _i->mountSystemPartition();
            _i->restoreBootFiles();

            emit statusUpdate(tr("Finish writing boot files to disk (sync)"));
            sync();
        }
    }

    emit statusUpdate(tr("Formatting data partition (%1)").arg(_fs));
    if (!formatDataPartition())
    {
        emit error(tr("Error Formatting data partition (%1)").arg(_fs));
        return;
    }

    if (_initializedata)
    {
        emit statusUpdate(tr("Mounting and initializing data partition"));
        _i->initializeDataPartition(_datadev);

        emit statusUpdate(tr("Editing cmdline.txt"));

        /* Data dev setting */
        QFile f("/boot/cmdline.txt");
        f.open(QIODevice::ReadWrite);
        QByteArray line = f.readAll().trimmed();
        if (_fs == "btrfs")
            line += " fstype=btrfs";
        if (_iscsi)
            line += " datadev=iscsi";
        else
            line += " datadev="+_datadev;
        f.seek(0);
        f.write(line.trimmed());
        f.close();

        /* Data dev setting in uEnv.txt (for A10 devices) */
        f.setFileName("/boot/uEnv.txt");
        f.open(QIODevice::ReadWrite);
        line = f.readAll().trimmed();
        if (_fs == "btrfs")
            line += " fstype=btrfs";
        if (_iscsi)
            line += " datadev=iscsi";
        else
            line += " datadev="+_datadev;
        f.seek(0);
        f.write(line.trimmed());
        f.close();

        /* Overscan setting */
        bool configchanged = false;
        f.setFileName("/boot/config.txt");
        f.open(QIODevice::ReadOnly);
        QByteArray configdata = f.readAll();
        f.close();
        bool overscanCurrentlyDisabled = configdata.contains("disable_overscan=1");

        if (_i->disableOverscan() && !overscanCurrentlyDisabled)
        {
            configdata += "\ndisable_overscan=1";
            configchanged = true;
        }
        else if (!_i->disableOverscan() && overscanCurrentlyDisabled)
        {
            configdata.replace("disable_overscan=1", "");
            configchanged = true;
        }
        if (configchanged)
        {
            f.open(QIODevice::WriteOnly);
            f.write(configdata.trimmed());
            f.close();
        }

        /* Finished */
        emit statusUpdate(tr("Unmounting boot partition"));
        _i->umountSystemPartition();

        emit statusUpdate(tr("Finish writing to disk (sync)"));
        sync();

        emit statusUpdate(tr("Mounting boot partition again"));
        _i->mountSystemPartition();
    }

    emit completed();
}

bool DriveFormatThread::partitionDrive()
{
    QByteArray partitionTable;

    if (_reformatBoot)
        partitionTable = "2048,129024,0E\n"; /* 63 MB FAT partition LBA */
//        partitionTable = "0,128,6\n"; /* 64 MB FAT partition */

    partitionTable += "131072,,L\n"; /* Linux partition with all remaining space */
    partitionTable += "0,0\n";
    partitionTable += "0,0\n";
    if (!_reformatBoot)
        partitionTable += "0,0\n";

    //QString cmd = QString("/sbin/sfdisk -H 32 -S 32 /dev/")+_dev;
    QString cmd = QString("/sbin/sfdisk -H 255 -S 63 -u S /dev/")+_dev;
    QProcess proc;
    proc.setProcessChannelMode(proc.MergedChannels);
    proc.start(cmd);
    proc.write(partitionTable);
    proc.closeWriteChannel();
    proc.waitForFinished(-1);
    //qDebug() << "partitioning" << cmd;
    //qDebug() << partitionTable;

    return proc.exitCode() == 0;
}

bool DriveFormatThread::formatBootPartition()
{
    return QProcess::execute(QString("/sbin/mkdosfs /dev/")+_bootdev) == 0;
}

bool DriveFormatThread::formatDataPartition()
{
    QString cmd;

    if (_fs == "btrfs")
        cmd = QString("/sbin/mkfs.btrfs -L berryboot /dev/")+_datadev;
    else if (_fs == "ext4")
        cmd = QString("/usr/sbin/mkfs.ext4 -L berryboot /dev/")+_datadev;
    else
        cmd = QString("/usr/sbin/mkfs.ext4 -E nodiscard -L berryboot /dev/")+_datadev;

    return QProcess::execute(cmd) == 0;
}

bool DriveFormatThread::zeroMbr()
{
    if (QFile::exists("/tmp/boot/mbr.bin"))
        return QProcess::execute("/bin/dd if=/tmp/boot/mbr.bin of=/dev/"+_dev) == 0;
    else
        return QProcess::execute("/bin/dd count=1 bs=512 if=/dev/zero of=/dev/"+_dev) == 0;
}

bool DriveFormatThread::installUbootSPL()
{
    return (QProcess::execute("/bin/dd bs=1024 seek=8 if=/tmp/boot/sunxi-spl.bin of=/dev/"+_dev) == 0)
        && (QProcess::execute("/bin/dd bs=1024 seek=32 if=/tmp/boot/u-boot.bin of=/dev/"+_dev) == 0);
}

QString DriveFormatThread::iscsiDevice()
{
    /* Scan for storage devices */
    QString dirname  = "/sys/class/block";
    QDir    dir(dirname);
    QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (QString dev, list)
    {
        if (QFile::symLinkTarget("/sys/class/block/"+dev).contains("/session"))
        {
            return dev;
        }
    }

    return "";
}

QString DriveFormatThread::drive()
{
    return _dev;
}

QString DriveFormatThread::bootdev()
{
    return _bootdev;
}

QString DriveFormatThread::datadev()
{
    return _datadev;
}
