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

DriveFormatThread::DriveFormatThread(const QString &drive, const QString &filesystem, Installer *i, QObject *parent, const QString &bootdev, bool initializedata, bool password) :
    QThread(parent), _dev(drive), _bootdev(bootdev), _fs(filesystem), _iscsi(false), _initializedata(initializedata), _password(password), _i(i)
{
    if (_dev == "iscsi")
    {
        _iscsi = true;
        for (int i=0; i<50; i++)
        {
            _dev = _i->iscsiDevice();
            if (!_dev.isEmpty())
                break;
            QThread::msleep(100);
        }
        _datadev = _dev;
    }
    else if (_dev.startsWith("sd") || _dev.startsWith("hd"))
        _datadev = _dev;
    else
        _datadev = _dev+"p"; // mmcblk0p1 instead of mmcblk01

    _reformatBoot = (_dev == "mmcblk0" || !_bootdev.startsWith("mmcblk0"));

    if (_iscsi)
        _datadev += "1";
    else
        _datadev += "2";
    /*
    if (_reformatBoot)
        _datadev += "2";
    else
        _datadev += "1";
    */
}

void DriveFormatThread::run()
{
    if (_reformatBoot)
    {
        emit statusUpdate(tr("Saving boot files to memory"));
        _i->cleanupDrivers();

        if (_i->sizeofBootFilesInKB() > SIZE_BOOT_PART * 1000)
        {
            emit error(tr("SD card contains extra files that do not belong to Berryboot. Please copy them to another disk and delete them from card."));
            return;
        }

        if (QFile::exists("/lib/modules"))
        {
            /* Delete extracted drivers to free up tmpfs space */
            QProcess::execute("rm -rf /lib/modules");
        }

        if (!_i->saveBootFiles() )
        {
            emit error(tr("Error saving boot files to memory. SD card may be damaged."));
            return;
        }
        if (_initializedata)
        {
            if (!_i->umountSystemPartition())
            {
                emit error(tr("Error unmounting system partition."));
                return;
            }
        }
    }

    if (_fs != "existing")
    {
        emit statusUpdate(tr("Zeroing partition table"));

        if (_iscsi && _dev.isEmpty())
        {
            emit error(tr("Unable to find my iSCSI device"));
            return;
        }

        if (!zeroMbr())
        {
            emit error(tr("Error zero'ing MBR/GPT of device '%1'. SD card may be broken or advertising wrong capacity.").arg(_dev) );
            return;
        }

        emit statusUpdate(tr("Creating partitions"));

        if (!partitionDrive())
        {
            emit error(tr("Error partitioning"));
            return;
        }
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
            //_i->mountSystemPartition();
            QProcess::execute("mount /dev/"+_bootdev+" /boot");
            _i->restoreBootFiles();

            emit statusUpdate(tr("Finish writing boot files to disk (sync)"));
            sync();
        }
    }
    else
    {
        QProcess::execute("/sbin/fatlabel /dev/"+_bootdev+" boot");
    }

    if (_fs != "existing")
    {
        emit statusUpdate(tr("Formatting data partition (%1)").arg(_fs));
        if (!formatDataPartition())
        {
            emit error(tr("Error Formatting data partition (%1)").arg(_fs));
            return;
        }
    }

    if (_initializedata)
    {
        emit statusUpdate(tr("Mounting and initializing data partition"));
        _i->initializeDataPartition(_password ? "mapper/luks" : _datadev);

        emit statusUpdate(tr("Editing cmdline.txt"));

        /* Data dev setting */
        QByteArray param;
        if (_fs == "btrfs" || (_fs == "existing" && isBtrfs()) )
            param += " fstype=btrfs";
        if (_iscsi)
            param += " datadev=iscsi";
        else if (_datadev == "mmcblk0p2")
            param += " datadev=mmcblk0p2";
        else
            param += " datadev="+_i->uuidOfDevice(_datadev.toLatin1());
        if (_password)
            param += " luks";
        if (_bootdev != "mmcblk0p1")
            param += " bootdev="+_i->uuidOfDevice(_bootdev.toLatin1());

        /* Static MAC setting */
        if (_i->fixateMAC())
        {
            QByteArray mac = _i->macAddress();
            if (!mac.isEmpty())
                param += " mac_addr="+mac;
        }

        /* Sound channel selection (hdmi audio/headphones) */
        if (!_i->sound().isEmpty())
        {
            param += " sound="+_i->sound();
        }

        QByteArray qmap = _i->keyboardlayout().toLatin1();
        if (!qmap.isEmpty() && qmap != "us")
            param += " qmap="+qmap;

        if (_i->skipConfig())
            param += " reconfigure";

        QFile f("/boot/cmdline.txt");
        f.open(QIODevice::ReadOnly);
        QByteArray line = f.readAll().trimmed();
        QByteArray cmdlinetxt = line+param;
        f.close();
        f.open(QIODevice::WriteOnly);
        f.write(cmdlinetxt);
        f.close();

        /* Data dev setting in uEnv.txt (for A10 devices) */
        f.setFileName("/boot/uEnv.txt");
        if (f.exists())
        {
            f.open(QIODevice::ReadWrite);
            line = f.readAll().trimmed();
            f.seek(0);
            f.write(line+param+"\n");
            f.close();
        }

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
        if (configdata.contains("\nprogram_usb_boot_mode=1"))
        {
            configdata.replace("program_usb_boot_mode=1", "");
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
        _i->cleanupDrivers();
        _i->umountSystemPartition();

        emit statusUpdate(tr("Finish writing to disk (sync)"));
        sync();

        /* Drop page cache */
        f.setFileName("/proc/sys/vm/drop_caches");
        f.open(f.WriteOnly);
        f.write("3\n");
        f.close();

        emit statusUpdate(tr("Mounting boot partition again"));
        //_i->mountSystemPartition();
        QProcess::execute("mount /dev/"+_bootdev+" /boot");

        /* Verify that cmdline.txt was written correctly */
        f.setFileName("/boot/cmdline.txt");
        f.open(f.ReadOnly);
        QByteArray cmdlineread = f.readAll();
        f.close();
        if (cmdlineread != cmdlinetxt)
        {
            emit error(tr("SD card broken (writes do not persist)"));
            return;
        }
    }

    emit completed();
}

bool DriveFormatThread::partitionDrive()
{
    QByteArray partitionTable;
    int size_boot_part_in_sectors = 2048 * SIZE_BOOT_PART;
    int start_main_part = size_boot_part_in_sectors + 2048;

    QFile f("/sys/class/block/"+_dev+"/size");
    f.open(f.ReadOnly);
    qulonglong blocks = f.readAll().trimmed().toULongLong();
    f.close();
    bool gpt = (blocks > 4294967295ULL);

    if (!_iscsi)
    {
        partitionTable = "2048,"+QByteArray::number(size_boot_part_in_sectors);

        if (gpt)
            partitionTable += ",U\n"; /* EFI system partition */
        else
            partitionTable += ",0E\n"; /* FAT partition LBA */
    }

    partitionTable += QByteArray::number(start_main_part)+",,L\n"; /* Linux partition with all remaining space */
    partitionTable += "0,0\n";
    partitionTable += "0,0\n";
    if (_iscsi)
        partitionTable += "0,0\n";

    //QString cmd = QString("/sbin/sfdisk -H 32 -S 32 /dev/")+_dev;
    //QString cmd = QString("/sbin/sfdisk -H 255 -S 63 -u S /dev/")+_dev;
    QString cmd = QString("/sbin/sfdisk -u S /dev/")+_dev;
    if (gpt)
        cmd += " --label gpt";
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
    return QProcess::execute(QString("/sbin/mkfs.fat -n boot /dev/")+_bootdev) == 0;
}

bool DriveFormatThread::formatDataPartition()
{
    QString cmd;
    QString dev;

    if (!_password)
    {
        dev = _datadev;
    }
    else
    {
        dev = "mapper/luks";
        _i->loadCryptoModules();
        _i->cleanupDrivers();

        /* For added security, let the cryptsetup program ask for the password in a text console */
        QProcess proc;
        proc.start(QByteArray("openvt -c 5 -w /usr/sbin/cryptsetup -q luksFormat /dev/")+_datadev);
        _i->switchConsole(5);
        proc.waitForFinished();

        if (proc.exitCode())
        {
            _i->switchConsole(1);
            return false;
        }

        proc.start(QByteArray("openvt -c 5 -w /usr/sbin/cryptsetup luksOpen /dev/")+_datadev+" luks");
        proc.waitForFinished();
        _i->switchConsole(1);

        if (proc.exitCode())
            return false;
    }

    if (_fs == "btrfs")
        cmd = QString("/usr/bin/mkfs.btrfs -f -L berryboot /dev/")+dev;
    else if (_fs == "ext4")
        cmd = QString("/sbin/mkfs.ext4 -O ^huge_file -L berryboot /dev/")+dev;
    else if (_fs == "ext4 nolazy")
        cmd = QString("/sbin/mkfs.ext4 -E lazy_itable_init=0,lazy_journal_init=0 -O ^huge_file -L berryboot /dev/")+dev;
    else
        cmd = QString("/sbin/mkfs.ext4 -E nodiscard -L berryboot /dev/")+dev;

    return QProcess::execute(cmd) == 0;
}

bool DriveFormatThread::zeroMbr()
{
    if (QFile::exists("/tmp/boot/mbr.bin"))
        return QProcess::execute("/bin/dd if=/tmp/boot/mbr.bin of=/dev/"+_dev) == 0;
    else
    {
        /* First 512 bytes should be enough to zero out the MBR, but we zero out 8 kb to make sure we also erase any
         * GPT primary header and get rid of any partitionless FAT headers.
         * Also zero out the last 4 kb of the card to get rid of any secondary GPT header
         *
         * Using conv=fsync to make sure we get notified of write errors
         */
        QFile f("/sys/class/block/"+_dev+"/size");
        f.open(f.ReadOnly);
        qulonglong blocks = f.readAll().trimmed().toULongLong();
        f.close();

        if (blocks < 8)
            return false;

        return QProcess::execute("/bin/dd conv=fsync count=1 bs=8192 if=/dev/zero of=/dev/"+_dev) == 0
            && QProcess::execute("/bin/dd conv=fsync count=8 bs=512 if=/dev/zero seek="+QString::number(blocks-8)+" of=/dev/"+_dev) == 0;
    }
}

bool DriveFormatThread::installUbootSPL()
{
    return (QProcess::execute("/bin/dd bs=1024 seek=8 if=/tmp/boot/sunxi-spl.bin of=/dev/"+_dev) == 0)
        && (QProcess::execute("/bin/dd bs=1024 seek=32 if=/tmp/boot/u-boot.bin of=/dev/"+_dev) == 0);
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

bool DriveFormatThread::isBtrfs()
{
    QProcess proc;
    proc.start("/sbin/blkid /dev/"+_datadev);
    if (proc.waitForFinished() && proc.exitCode() == 0)
    {
        QByteArray res = proc.readAll();

        if (res.contains("TYPE=\"btrfs\""))
        {
            return true;
        }
    }

    return false;
}
