/* Berryboot -- installer utility class
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


#include "installer.h"
#include "ceclistener.h"
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QTime>
#include <QApplication>

#define HAVE_SYS_STATVFS_H 1
#define HAVE_STATVFS 1
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/reboot.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <blkid/blkid.h>

#ifdef Q_WS_QWS
#include <QWSServer>
#include <QKbdDriverFactory>
#endif

/* Magic to test if image is a valid SquashFS file */
#define SQUASHFS_MAGIC			0x73717368
#define SQUASHFS_MAGIC_SWAP		0x68737173

Installer::Installer(QObject *parent) :
    QObject(parent), _ethup(false), _skipConfig(false), _settings(NULL)
{
}

bool Installer::saveBootFiles()
{
    return QProcess::execute("cp -a /boot /tmp") == 0;
}

bool Installer::restoreBootFiles()
{
    bool status = QProcess::execute("cp -a /tmp/boot /") == 0;
    QProcess::execute("rm -rf /tmp/boot");
    return status;
}

int Installer::sizeofBootFilesInKB()
{
    QProcess proc;
    proc.start("du -s /boot");
    proc.waitForFinished();
    return proc.readAll().split('\t').first().toInt();
}

void Installer::setBootoptions(const QByteArray &newOptions)
{
    _bootoptions = newOptions;
}

QByteArray Installer::bootoptions()
{
    if (_bootoptions.isEmpty())
    {
        QFile f("/proc/cmdline");
        f.open(f.ReadOnly);
        QByteArray cmdline = f.readAll();
        f.close();
        _bootoptions = cmdline.trimmed();
    }

    return _bootoptions;
}

QByteArray Installer::bootParam(const QByteArray &name)
{
    /* Search for key=value in /proc/cmdline */
    QByteArray line = bootoptions();
    QByteArray searchFor = name+"=";
    int searchForLen = searchFor.length();

    int pos = line.indexOf(searchFor);
    if (pos == -1)
    {
        return "";
    }
    else
    {
        int end;

        if (line.length() > pos+searchForLen && line.at(pos+searchForLen) == '"')
        {
            /* Value between quotes */
            searchForLen++;
            end = line.indexOf('"', pos+searchForLen);
        }
        else
        {
            end = line.indexOf(' ', pos+searchForLen);
        }
        if (end != -1)
            end = end-pos-searchForLen;
        return line.mid(pos+searchForLen, end);
    }
}

QByteArray Installer::datadev()
{
    return bootParam("datadev");
}

QByteArray Installer::bootdev()
{
    if (_bootdev.isEmpty())
    {
        _bootdev = bootParam("bootdev");
        if (_bootdev.isEmpty())
        {
            if (QFile::exists("/dev/mmcblk0"))
            {
                _bootdev = "mmcblk0p1";
            }
            else
            {
                QTime t;
                t.start();

                while (_bootdev.isEmpty() && t.elapsed() < 10000)
                {
                    QApplication::processEvents(QEventLoop::WaitForMoreEvents, 100);
                    _bootdev = findBootPart();
                }
            }
        } else if (_bootdev.contains('=')) {
            _bootdev = getPartitionByUuid(_bootdev);
        }
    }

    return _bootdev;
}

QByteArray Installer::findBootPart()
{
    /* Search for partition with berryboot.img */
    QByteArray part;
    QString dirname  = "/sys/class/block";
    QDir    dir(dirname);
    QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (QString devname, list)
    {
        /* Only search first partition and partitionless devices. Skip virtual devices (such as ramdisk) */
        if ((devname.right(1).at(0).isDigit() && !devname.endsWith("1"))
                || QFile::symLinkTarget("/sys/class/block/"+devname).contains("/devices/virtual/"))
            continue;

        if (QProcess::execute("mount -t vfat -o ro /dev/"+devname+" /boot") == 0)
        {
            if (QFile::exists("/boot/berryboot.img"))
            {
                qDebug() << "Found berryboot.img at" << devname;
                part = devname.toLatin1();
            }

            QProcess::execute("umount /boot");
        }

        if (!part.isEmpty())
            break;
    }

    return part;
}

void Installer::initializeDataPartition(const QString &dev)
{
    if (QProcess::execute("mount /dev/"+dev+" /mnt") != 0)
    {
        log_error(tr("Error mounting data partition"));
        return;
    }

    QDir dir;
    dir.mkdir("/mnt/images");
    dir.mkdir("/mnt/data");
    dir.mkpath("/mnt/shared/etc/default");
    dir.mkpath("/mnt/shared/lib/udev/rules.d");
    dir.mkdir("/mnt/tmp");

    QFileInfo fi("/boot/shared.tgz");
    if (fi.exists() && fi.size())
    {
        int result = system("/bin/gzip -dc /boot/shared.tgz | /bin/tar x -C /mnt/shared");
        if (result != 0)
            log_error(tr("Error extracting shared.tgz ")+QString::number(result) );
    }
    fi.setFile("/boot/shared.img");
    if (fi.exists())
    {
        int result = system("/usr/bin/unsquashfs -d /mnt/shared -f /boot/shared.img");
        if (result != 0)
            log_error(tr("Error extracting shared.img ")+QString::number(result) );
    }

    if (!_timezone.isEmpty())
    {
        //symlink(_timezone, "/shared/")
        QFile f("/mnt/shared/etc/timezone");
        f.open(f.WriteOnly);
        f.write(_timezone.toLatin1()+"\n");
        f.close();
    }
    if (!_keyboardlayout.isEmpty())
    {
        QByteArray keybconfig =
            "XKBMODEL=\"pc105\"\n"
            "XKBLAYOUT=\""+_keyboardlayout.toLatin1()+"\"\n"
            "XKBVARIANT=\"\"\n"
            "XKBOPTIONS=\"\"\n";

        QFile f("/mnt/shared/etc/default/keyboard");
        f.open(f.WriteOnly);
        f.write(keybconfig);
        f.close();
    }
    if (QFile::exists("/boot/wpa_supplicant.conf"))
    {
        dir.mkdir("/mnt/shared/etc/wpa_supplicant");
        QFile::copy("/boot/wpa_supplicant.conf", "/mnt/shared/etc/wpa_supplicant/wpa_supplicant.conf");
        QFile::setPermissions("/mnt/shared/etc/wpa_supplicant/wpa_supplicant.conf", QFile::ReadOwner | QFile::WriteOwner);
    }
    /* Tell udisks not to auto-mount Berryboot partition */
    QFile fu("/mnt/shared/lib/udev/rules.d/80-berryboot.rules");
    fu.open(fu.WriteOnly);
    fu.write("ENV{ID_FS_LABEL}==\"berryboot\", ENV{UDISKS_IGNORE}=\"1\"\n");
    fu.close();

    if (!bootParam("ipv4").isEmpty())
    {
        QByteArray dns = bootParam("dns");
        QDir dir;

        if (dns.isEmpty())
            dns = "8.8.8.8";

        dir.mkdir("/mnt/shared/etc/network");
        QByteArray ifaces = "# Static network configuration handled by Berryboot\n"
                "iface eth0 inet manual\n\n"
                "auto lo\n"
                "iface lo inet loopback\n";
        QFile f("/mnt/shared/etc/network/interfaces");
        f.open(f.WriteOnly);
        f.write(ifaces);
        f.close();

        f.setFileName("/mnt/shared/etc/resolv.conf");
        f.open(f.WriteOnly);
        f.write("nameserver "+dns);
        f.close();
    }

    QSettings *s = settings();
    s->beginGroup("berryboot");
    if (s->contains("fschmod"))
    {
        if (::chown("/mnt", s->value("fsuid", 0).toInt(), s->value("fsguid", 0).toInt()) != 0) { }
        ::chmod("/mnt", s->value("fschmod").toString().toInt(0, 8));
    }
    s->endGroup();
}

bool Installer::mountSystemPartition()
{
    if (isPxeBoot())
        return true;

    return QProcess::execute("mount /dev/"+bootdev()+" /boot") == 0;
}

void Installer::startNetworking()
{
    QFile f("/sys/class/net/eth0/carrier");
    QByteArray carrier;

    if (!f.exists())
    {
        /* eth0 not available yet, check back in a tenth of a second */
        QTimer::singleShot(100, this, SLOT(startNetworking()));
        return;
    }

    f.open(f.ReadOnly);
    carrier = f.readAll().trimmed();
    f.close();
    if (carrier.isEmpty() && !_ethup)
    {
        QProcess::execute("/sbin/ifconfig eth0 up");
        _ethup = true;
    }
    if (carrier != "1")
    {
        /* check back in a tenth of a second */
        QTimer::singleShot(100, this, SLOT(startNetworking()));
        return;
    }

    QProcess *proc = new QProcess(this);
    connect(proc, SIGNAL(finished(int)), SLOT(ifupFinished(int)));
    proc->start("/sbin/ifup eth0");
}

/* Only enable interface, but do not obtain DHCP lease */
void Installer::startNetworkInterface()
{
    if (_ethup)
        return;

    QFile f("/sys/class/net/eth0");

    if (!f.exists())
    {
        /* eth0 not available yet, check back in a tenth of a second */
        QTimer::singleShot(100, this, SLOT(startNetworkInterface()));
        return;
    }

    QProcess::startDetached("/sbin/ifconfig eth0 up");
    _ethup = true;
}

bool Installer::networkReady()
{
    /* Once we have a DHCP lease /tmp/resolv.conf is created */
    QFile f("/tmp/resolv.conf");
    return f.exists() && f.size() > 0;
}

bool Installer::eth0Up()
{
    return _ethup;
}

bool Installer::umountSystemPartition()
{
    if (isPxeBoot())
        return true;

    if ( QProcess::execute("umount /boot") != 0)
    {
        log_error(tr("Error unmounting system partition"));
        return false;
    }

    return true;
}

void Installer::log_error(const QString &msg)
{
    emit error(msg);
}

void Installer::ifupFinished(int)
{
    QProcess *p = qobject_cast<QProcess*> (sender());

    emit networkInterfaceUp();
    p->deleteLater();
}

double Installer::availableDiskSpace(const QString &path)
{
        double bytesfree = 0;
        struct statvfs buf;

        QByteArray pathba = path.toLatin1();
        if (statvfs(pathba.constData(), &buf)) {
                return -1;
        }
        if (buf.f_frsize) {
                bytesfree = (((double)buf.f_bavail) * ((double)buf.f_frsize));
        } else {
                bytesfree = (((double)buf.f_bavail) * ((double)buf.f_bsize));
        }

        return bytesfree;
}

double Installer::diskSpaceInUse(const QString &path)
{
        double bytes = 0;
        struct statvfs buf;

        QByteArray pathba = path.toLatin1();
        if (statvfs(pathba.constData(), &buf)) {
                return -1;
        }
        if (buf.f_frsize) {
                bytes = (((double)(buf.f_blocks-buf.f_ffree)) * ((double)buf.f_frsize));
        } else {
                bytes = (((double)(buf.f_blocks-buf.f_ffree)) * ((double)buf.f_bsize));
        }

        return bytes;
}

void Installer::reboot()
{
    QProcess::execute("killall udevd");
    ::usleep(100000);
    QProcess::execute("umount -ar");
    sync();
    QProcess::execute("ifdown -a");
    ::reboot(RB_AUTOBOOT);
}

QMap<QString,QString> Installer::listInstalledImages()
{
    QMap<QString,QString> images;
    QDir dir("/mnt/images");
    QStringList list = dir.entryList(QDir::Files);

    foreach (QString image,list)
    {
        images[image] = imageFilenameToFriendlyName(image);
    }

    return images;
}

QString Installer::imageFilenameToFriendlyName(const QString &name)
{
    QString n = name;

    /* Replace underscores with spaces */
    n.replace('_', ' ');

    /* If name is a full URL, extract filename */
    int slashpos = n.lastIndexOf('/');
    if (slashpos != -1)
        n = n.mid(slashpos+1);

    /* Chop .img extension off */
    int imgpos = n.lastIndexOf(".img");
    if (imgpos != -1)
        n = n.mid(0, imgpos);

    return n;
}

QString Installer::getDefaultImage()
{
    QString result;
    QFile f("/mnt/data/default");

    if (f.exists())
    {
        f.open(f.ReadOnly);
        result = f.readAll();
        f.close();

        if (!QFile::exists("/mnt/images/"+result))
            result = "";
    }

    return result;
}

void Installer::setDefaultImage(const QString &name)
{
    QFile f("/mnt/data/default");

    if (name.isEmpty())
        f.remove();
    else
    {
        f.open(f.WriteOnly);
        f.write(name.toLatin1());
        f.close();
    }
}

void Installer::renameImage(const QString &oldname, const QString &newName)
{
    if (getDefaultImage() == oldname)
        setDefaultImage(newName);

    QFile::rename("/mnt/images/"+oldname, "/mnt/images/"+newName);
    QFile::rename("/mnt/data/"+oldname, "/mnt/data/"+newName);
}

void Installer::deleteImage(const QString &name)
{
    if (name.isEmpty())
        return;

    QString datadir = "/mnt/data/"+name;
    QString workdir = datadir+".work";
    bool wasDefaultImage = (getDefaultImage() == name);

    if (QFile::exists(datadir))
    {
        QStringList param;
        param << "-rf" << datadir;

        if (QFile::exists(workdir))
            param << workdir;

        QProcess::execute("rm", param); /* TODO write proper Qt function for recursive delete*/
    }
    QFile::remove("/mnt/images/"+name);

    if (wasDefaultImage)
    {
        QMap<QString,QString> images = listInstalledImages();
        if (images.empty())
            setDefaultImage("");
        else
            setDefaultImage(images.keys().first() );
    }
}

void Installer::deleteUserChanges(const QString &name)
{
    if (name.isEmpty())
        return;

    QStringList param;
    param << "-rf" << "/mnt/data/"+name;
    QProcess::execute("rm", param); /* TODO write proper Qt function for recursive delete*/
}

void Installer::cloneImage(const QString &oldname, const QString &newname, bool clonedata)
{
    QByteArray oldnamepath = QByteArray("/mnt/images/")+oldname.toLatin1();
    QByteArray newnamepath = QByteArray("/mnt/images/")+newname.toLatin1();

    if (link(oldnamepath.constData(), newnamepath.constData()) == 0 && clonedata)
    {
        if (QFile::exists("/mnt/data/"+oldname))
        {
            QDir dir;
            dir.mkdir("/mnt/data/"+newname);

            QByteArray cmd = QByteArray("cp -a /mnt/data/")+oldname.toLatin1()+"/* /mnt/data/"+newname.toLatin1();
            if (system(cmd.constData()) != 0)
                log_error("Error copying modified data");
            // TODO: BTRFS reflink?
        }
    }
}

void Installer::setKeyboardLayout(const QString &layout)
{
    if (layout != _keyboardlayout)
    {
        _keyboardlayout = layout;

#ifdef Q_WS_QWS
        QString keymapfile = ":/qmap/"+layout;

        if (QFile::exists(keymapfile))
        {
            qDebug() << "Changing keymap to:" << keymapfile;

            // Loading keymaps from resources directly is broken, so copy to /tmp first
            QFile::copy(keymapfile, "/tmp/"+layout);
            keymapfile = "/tmp/"+layout;

            QWSServer *q = QWSServer::instance();
            q->closeKeyboard();
            q->setKeyboardHandler(QKbdDriverFactory::create("TTY", "keymap="+keymapfile));
        }
        else
        {
            qDebug() << "Keyboard driver not found:" << layout;
        }
#endif
    }
}

void Installer::setTimezone(const QString &tz)
{
    _timezone = tz;
}

void Installer::setSkipConfig(bool skip)
{
    _skipConfig = skip;
}

QString Installer::timezone() const
{
    return _timezone;
}

QString Installer::keyboardlayout() const
{
    return _keyboardlayout;
}

bool Installer::skipConfig() const
{
    return _skipConfig;
}

void Installer::setDisableOverscan(bool disabled)
{
    _disableOverscan = disabled;
}

bool Installer::disableOverscan() const
{
    return _disableOverscan;
}

void Installer::setFixateMAC(bool fix)
{
    _fixMAC = fix;
}

bool Installer::fixateMAC() const
{
    return _fixMAC;
}

void Installer::setSound(const QByteArray &sound)
{
    _sound = sound;
}

QByteArray Installer::sound() const
{
    return _sound;
}

bool Installer::isSquashFSimage(QFile &f)
{
    quint32 magic = 0;

    f.open(f.ReadOnly);
    f.read( (char *) &magic, sizeof(magic));
    f.close();

    return (magic == SQUASHFS_MAGIC || magic == SQUASHFS_MAGIC_SWAP);
}

void Installer::prepareDrivers()
{
    if ( !QFile::exists("/lib/modules") )
    {
        if (QFile::exists("/mnt/shared/lib/modules"))
        {
            /* Use shared modules from disk */
            if (symlink("/mnt/shared/lib/modules", "/lib/modules")
             || symlink("/mnt/shared/lib/firmware", "/lib/firmware"))
            {
                // show error?
            }
        }
        else if (QFile::exists("/mnt/shared/usr/lib/modules"))
        {
            /* Use shared modules from disk */
            if (symlink("/mnt/shared/usr/lib/modules", "/lib/modules")
             || symlink("/mnt/shared/usr/lib/firmware", "/lib/firmware"))
            {
                // show error?
            }
        }
        else if (QFile::exists("/boot/shared.img"))
        {
            /* Not yet installed, but shared.img squahsfs file with drivers available */
            QDir dir;
            dir.mkdir("/mnt_shared_img");
            QProcess::execute("mount -o loop,ro /boot/shared.img /mnt_shared_img");
            if (symlink("/mnt_shared_img/lib/modules", "/lib/modules")
             || symlink("/mnt_shared_img/lib/firmware", "/lib/firmware"))
            {
                // show error?
            }
        }
        else
        {
            /* Not yet installed, uncompress shared.tgz from boot partition into ramfs */
            if (system("gzip -dc /boot/shared.tgz | tar x -C /") != 0) { }
        }
    }
}

void Installer::cleanupDrivers()
{
    QDir dir;

    if (dir.exists("/mnt_shared_img"))
    {
        QProcess::execute("killall udevd");
        ::usleep(100000);
        QProcess::execute("umount /mnt_shared_img");
        dir.rmdir("/mnt_shared_img");
        dir.rmdir("/lib/modules");
        dir.rmdir("/lib/firmware");
    }
}

void Installer::loadDrivers()
{
    prepareDrivers();

    if (QFile::exists("/sbin/udevd"))
    {
        if (QProcess::execute("pidof udevd") != 0)
        {
            QFile f("/proc/sys/kernel/hotplug");
            f.open(f.WriteOnly);
            f.write("\0\0\0\0");
            f.close();

            QProcess::execute("/sbin/udevd --daemon");
            QProcess::execute("/sbin/udevadm trigger");
        }
    }
    else
    {
        /* Tell the kernel to contact our /sbin/hotplug helper script,
           if a module wants firmware to be loaded */
        QFile f("/proc/sys/kernel/hotplug");
        f.open(f.WriteOnly);
        f.write("/sbin/hotplug\n");
        f.close();

        /* Load drivers for USB devices found */
        QString dirname  = "/sys/bus/usb/devices";
        QDir    dir(dirname);
        QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        foreach (QString dev, list)
        {
            QString modalias_file = dirname+"/"+dev+"/modalias";
            if (QFile::exists(modalias_file))
            {
                f.setFileName(modalias_file);
                f.open(f.ReadOnly);
                QString module = f.readAll().trimmed();
                f.close();
                QProcess::execute("/sbin/modprobe "+module);
            }
        }
    }
}

void Installer::loadCryptoModules()
{
    prepareDrivers();

    QProcess::execute("/sbin/modprobe dm_crypt");
    QProcess::execute("/sbin/modprobe aes");
    QProcess::execute("/sbin/modprobe sha256");
    QProcess::execute("/sbin/modprobe aes_arm_bs");
    QProcess::execute("/sbin/modprobe aes_arm");
    QProcess::execute("/sbin/modprobe xts");
    QProcess::execute("/sbin/modprobe hmac");
    QProcess::execute("/sbin/modprobe algif_hash");
    QProcess::execute("/sbin/modprobe algif_skcipher");
}

void Installer::loadSoundModule(const QByteArray &channel)
{
    if (cpuinfo().contains("BCM2708") || cpuinfo().contains("BCM2709") || cpuinfo().contains("BCM2835"))
    {
        /* Raspberry Pi */
        prepareDrivers();
        QProcess::execute("/sbin/modprobe snd-bcm2835");

        if (channel == "headphones")
        {
            QProcess::execute("amixer cset numid=3 1");
        }
        else if (channel == "hdmi")
        {
            QProcess::execute("amixer cset numid=3 2");
        }
    }
}

void Installer::loadFilesystemModule(const QByteArray &fs)
{
    /* Only load module if fs is not already supported */
    QFile f("/proc/filesystems");
    f.open(f.ReadOnly);
    QByteArray fsSupported = f.readAll();
    f.close();

    if (!fsSupported.contains(fs))
    {
        prepareDrivers();
        QProcess::execute("/sbin/modprobe "+fs);
    }
}

void Installer::startWifi()
{
    loadDrivers();
    /* Wait up to 4 seconds for wifi device to appear */
    QTime t;
    t.start();
    while (t.elapsed() < 4000 && !QFile::exists("/sys/class/net/wlan0") )
    {
        QApplication::processEvents(QEventLoop::WaitForMoreEvents, 250);
    }

    QProcess::execute("/usr/sbin/wpa_supplicant -Dnl80211,wext -iwlan0 -c/boot/wpa_supplicant.conf -B");

    QProcess *p = new QProcess(this);
    connect(p, SIGNAL(finished(int)), this, SLOT(wifiStarted(int)));

    if (bootParam("ipv4").endsWith("/wlan0"))
        /* Using static configuration for wifi */
        p->start("/sbin/ifup wlan0");
    else
        p->start("/sbin/udhcpc -i wlan0 -O mtu");
}

void Installer::wifiStarted(int rc)
{
    if (rc != 0)
    {
        QMessageBox::critical(NULL, "Error connecting", "Error connecting to wifi. Check settings in /boot/wpa_supplicant.conf", QMessageBox::Ok);
    }

    sender()->deleteLater();
    emit networkInterfaceUp();
}

void Installer::enableCEC()
{
    QFile f("/proc/cpuinfo");
    f.open(f.ReadOnly);
    QByteArray cpuinfo = f.readAll();
    f.close();

    if (cpuinfo.contains("BCM2708") || cpuinfo.contains("BCM2709") || cpuinfo.contains("BCM2835")) /* Only supported on the Raspberry for now */
    {
        CecListener *cec = new CecListener(this);
        connect(cec, SIGNAL(keyPress(int)), this, SLOT(onKeyPress(int)));
        cec->start();
    }
}

/* Key on TV remote pressed */
void Installer::onKeyPress(int key)
{
#ifdef Q_WS_QWS
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    /* Map left/right cursor key to tab */
    switch (key)
    {
    case Qt::Key_Left:
        key = Qt::Key_Tab;
        modifiers = Qt::ShiftModifier;
        break;
    case Qt::Key_Right:
        key = Qt::Key_Tab;
        break;
    case Qt::Key_F1:
        key = Qt::Key_A;
        modifiers = Qt::ControlModifier;
        break;
    default:
        break;
    }

    // key press
    QWSServer::sendKeyEvent(0, key, modifiers, true, false);
    // key release
    QWSServer::sendKeyEvent(0, key, modifiers, false, false);
#else
    qDebug() << "onKeyPress" << key;
#endif
}

QSettings *Installer::settings()
{
    if (!_settings)
        _settings = new QSettings("/boot/berryboot.ini", QSettings::IniFormat);

    return _settings;
}

bool Installer::hasSettings()
{
    return QFile::exists("/boot/berryboot.ini");
}

/* Returns true if Berryboot is responsonsible for changing memsplit settings in config.txt
 * This is the case when the kernel either doesn't support CMA, or it is not enabled
 */
bool Installer::isMemsplitHandlingEnabled()
{
    if (!cpuinfo().contains("BCM2708"))
        return false; /* Not a Raspberry Pi. Current do not support memsplit changing on other devices */

    QFile f("/proc/vc-cma");
    if (!f.exists())
        return true; /* Raspberry Pi kernel without CMA support */

    f.open(f.ReadOnly);
    QByteArray cmainfo = f.readAll();
    f.close();

    return cmainfo.contains("Length     : 00000000");
    /* FIXME: figure out if there is an IOCTL or cleaner way to programaticcaly test if CMA is enabled */
}

bool Installer::hasOverscanSettings()
{
    /* Raspberry Pi has overscan settings */
    return cpuinfo().contains("BCM2708") || cpuinfo().contains("BCM2709");
}

bool Installer::hasDynamicMAC()
{
    /* Allwinner devices lack a static MAC address by default */
    QByteArray cpu = cpuinfo();
    return cpu.contains("sun4i") || cpu.contains("sun5i");
}

QByteArray Installer::cpuinfo()
{
    QFile f("/proc/cpuinfo");
    f.open(f.ReadOnly);
    QByteArray data = f.readAll();
    f.close();

    return data;
}

QByteArray Installer::macAddress()
{
    QFile f("/sys/class/net/eth0/address");
    f.open(f.ReadOnly);
    QByteArray data = f.readAll().trimmed();
    f.close();

    return data;
}

void Installer::switchConsole(int ttynr)
{
    QFile f("/dev/tty0");

    if (f.open(f.ReadWrite))
    {
        ioctl(f.handle(), VT_ACTIVATE, ttynr);
        f.close();
    }
    else
    {
        qDebug() << "Error opening tty";
    }
}

QByteArray Installer::model()
{
    QFile f("/proc/device-tree/model");

    f.open(f.ReadOnly);
    QByteArray data = f.readAll();
    f.close();

    return data;
}

bool Installer::supportsUSBboot()
{
    if (model().contains("Pi 3"))
    {
        QFile f("/boot/config.txt");
        f.open(f.ReadOnly);
        QByteArray config = f.readAll();
        f.close();

        return config.contains("\nprogram_usb_boot_mode=1");
    }
    else
    {
        return !QFile::exists("/dev/mmcblk0") && !QFile::exists("/dev/mmcblk0p1");
    }
}

QString Installer::iscsiDevice()
{
    /* Scan for storage devices */
    QString dirname  = "/sys/class/block";
    QDir    dir(dirname);
    QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (QString dev, list)
    {
        if (QFile::symLinkTarget("/sys/class/block/"+dev).contains("/session")
                && !QFile::exists("/sys/class/block/"+dev+"/partition"))
        {
            return dev;
        }
    }

    return "";
}

bool Installer::isPxeBoot()
{
    return !bootParam("iscsiscript").isEmpty();
}

QByteArray Installer::uuidOfDevice(QByteArray device)
{
    QByteArray uuid = device;

    if (!device.startsWith("/dev/"))
        device = "/dev/"+device;

    char *uuid_cstr = blkid_get_tag_value(NULL, "UUID", device.constData());

    if (uuid_cstr)
    {
        uuid = QByteArray("UUID=")+QByteArray(uuid_cstr);
        free(uuid_cstr);
    }

    return uuid;
}

QByteArray Installer::getDeviceByLabel(const QByteArray &label)
{
    QByteArray dev;
    char *cstr = blkid_get_devname(NULL, "LABEL", label.constData());

    if (cstr)
    {
        dev = cstr;
        free(cstr);
    }

    return dev;
}

QByteArray Installer::getDeviceByUuid(const QByteArray &uuid)
{
    QByteArray dev;
    char *cstr;

    if (uuid.contains('='))
        cstr = blkid_get_devname(NULL, uuid.constData(), NULL);
    else
        cstr = blkid_get_devname(NULL, "UUID", uuid.constData());

    if (cstr)
    {
        dev = cstr;
        free(cstr);
    }

    return dev;
}

QByteArray Installer::getPartitionByLabel(const QByteArray &label)
{
    QByteArray p = getDeviceByLabel(label);
    if (p.size()>5) /* remove /dev */
        p = p.mid(5);

    return p;
}

QByteArray Installer::getPartitionByUuid(const QByteArray &uuid)
{
    QByteArray p = getDeviceByUuid(uuid);
    if (p.size()>5) /* remove /dev */
        p = p.mid(5);

    return p;
}
