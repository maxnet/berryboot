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

#define HAVE_SYS_STATVFS_H 1
#define HAVE_STATVFS 1
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/reboot.h>

#ifdef Q_WS_QWS
#include <QWSServer>
#endif

/* Magic to test if image is a valid SquashFS file */
#define SQUASHFS_MAGIC			0x73717368
#define SQUASHFS_MAGIC_SWAP		0x68737173

Installer::Installer(QObject *parent) :
    QObject(parent), _settings(NULL)
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

QString Installer::datadev()
{
    QFile f("/proc/cmdline");
    f.open(f.ReadOnly);
    QByteArray cmdline = f.readAll();
    f.close();

    int pos = cmdline.indexOf("datadev=");
    if (pos == -1)
    {
        return "";
    }
    else
    {
        int end = cmdline.indexOf(' ', pos+8);
        if (end != -1)
            end = end-pos-8;
        return cmdline.mid(pos+8, end);
    }
}

bool Installer::mountDataPartition(const QString &dev)
{

    //if (execute("mount LABEL=berryboot /mnt") != 0)

    if (QProcess::execute("mount /dev/"+dev+" /mnt") != 0)
        return false;

    if (!QFile::exists("/mnt/images"))
    {
        if ( QProcess::execute("umount /mnt") != 0)
        {
            log_error(tr("Error unmounting data partition"));
        }
        return false;
    }

    return true;
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
    dir.mkdir("/mnt/shared");
    dir.mkdir("/mnt/shared/etc");
    dir.mkdir("/mnt/shared/etc/default");
    dir.mkdir("/mnt/tmp");
    int result = system("/bin/gzip -dc /boot/shared.tgz | /bin/tar x -C /mnt/shared");
    if (result != 0)
        log_error(tr("Error extracting shared.tgz ")+QString::number(result) );

    if (!_timezone.isEmpty())
    {
        //symlink(_timezone, "/shared/")
        QFile f("/mnt/shared/etc/timezone");
        f.open(f.WriteOnly);
        f.write(_timezone.toAscii()+"\n");
        f.close();
    }
    if (!_keyboardlayout.isEmpty())
    {
        QByteArray keybconfig =
            "XKBMODEL=\"pc105\"\n"
            "XKBLAYOUT=\""+_keyboardlayout.toAscii()+"\"\n"
            "XKBVARIANT=\"\"\n"
            "XKBOPTIONS=\"\"\n";

        QFile f("/mnt/shared/etc/default/keyboard");
        f.open(f.WriteOnly);
        f.write(keybconfig);
        f.close();
    }
    if (QFile::exists("/boot/wpa_supplicant.conf"))
    {
        QFile::copy("/boot/wpa_supplicant.conf", "/mnt/shared/etc/wpa_supplicant.conf");
    }
}

bool Installer::mountSystemPartition()
{
    return QProcess::execute("mount /dev/mmcblk0p1 /boot") == 0;
}

void Installer::startNetworking()
{
    QProcess *proc = new QProcess(this);
    connect(proc, SIGNAL(finished(int)), SLOT(ifupFinished(int)));
    proc->start("/sbin/ifup eth0");
}

bool Installer::networkReady()
{
    /* Once we have a DHCP release /tmp/resolv.conf is created */
    return QFile::exists("/tmp/resolv.conf");
}

void Installer::umountSystemPartition()
{
    if ( QProcess::execute("umount /boot") != 0)
    {
        log_error(tr("Error unmounting system partition"));
    }
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

        QByteArray pathba = path.toAscii();
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

        QByteArray pathba = path.toAscii();
        if (statvfs(pathba.constData(), &buf)) {
                return -1;
        }
        if (buf.f_frsize) {
                bytes = (((double)buf.f_blocks) * ((double)buf.f_frsize));
        } else {
                bytes = (((double)buf.f_blocks) * ((double)buf.f_bsize));
        }

        return bytes;
}

void Installer::reboot()
{
    if (system("umount -ar") != 0) { }
    sync();
    if (system("ifdown -a") != 0) { }
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
        f.write(name.toAscii());
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

    QStringList param;
    bool wasDefaultImage = (getDefaultImage() == name);
    param << "-rf" << "/mnt/data/"+name;
    QProcess::execute("rm", param); /* TODO write proper Qt function for recursive delete*/
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

void Installer::cloneImage(const QString &oldname, const QString &newname, bool clonedata)
{
    QByteArray oldnamepath = QByteArray("/mnt/images/")+oldname.toAscii();
    QByteArray newnamepath = QByteArray("/mnt/images/")+newname.toAscii();

    if (link(oldnamepath.constData(), newnamepath.constData()) == 0 && clonedata)
    {
        if (QFile::exists("/mnt/data/"+oldname))
        {
            QDir dir;
            dir.mkdir("/mnt/data/"+newname);

            QByteArray cmd = QByteArray("cp -a /mnt/data/")+oldname.toAscii()+"/* /mnt/data/"+newname.toAscii();
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
/*        QByteArray keymapfile = ":/qmap/"+layout.toAscii();

        if (QFile::exists(keymapfile))
        {
            QByteArray kbdarg = "TTY:keymap="+keymapfile;
            qDebug() << "Changing keyboard driver to:" << kbdarg;

            QWSServer *q = QWSServer::instance();
            q->closeKeyboard();
            q->setDefaultKeyboard(kbdarg.constData());
            q->openKeyboard();
        }
        else
        {
            qDebug() << "Keyboard driver not found:" << layout;
        } */
#endif
    }
}

void Installer::setTimezone(const QString &tz)
{
    _timezone = tz;
}

QString Installer::timezone() const
{
    return _timezone;
}
QString Installer::keyboardlayout() const
{
    return _keyboardlayout;
}

void Installer::setDisableOverscan(bool disabled)
{
    _disableOverscan = disabled;
}

bool Installer::disableOverscan() const
{
    return _disableOverscan;
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
        else
        {
            /* Not yet installed, uncompress shared.tgz from boot partition into ramfs */
            if (system("gzip -dc /boot/shared.tgz | tar x -C /") != 0) { }
        }
    }
}

void Installer::loadDrivers()
{
    prepareDrivers();

    QString dirname  = "/sys/bus/usb/devices";
    QDir    dir(dirname);
    QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (QString dev, list)
    {
        QString modalias_file = dirname+"/"+dev+"/modalias";
        if (QFile::exists(modalias_file))
        {
            QFile f(modalias_file);
            f.open(f.ReadOnly);
            QString module = f.readAll().trimmed();
            f.close();
            QProcess::execute("/sbin/modprobe "+module);
        }
    }
}

void Installer::startWifi()
{
    loadDrivers();
    QProcess::execute("/usr/sbin/wpa_supplicant -Dwext -iwlan0 -c/boot/wpa_supplicant.conf -B");

    if ( QProcess::execute("/sbin/udhcpc -n -i wlan0") != 0 )
    {
        QMessageBox::critical(NULL, "Error connecting", "Error connecting to wifi. Check settings in /boot/wpa_supplicant.conf", QMessageBox::Ok);
    }
}

void Installer::enableCEC()
{
    QFile f("/proc/cpuinfo");
    f.open(f.ReadOnly);
    QByteArray cpuinfo = f.readAll();
    f.close();

    if (cpuinfo.contains("BCM2708")) /* Only supported on the Raspberry for now */
    {
        CecListener *cec = new CecListener(this);
        connect(cec, SIGNAL(keyPress(int)), this, SLOT(onKeyPress(int)));
        cec->start();
    }
}

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
