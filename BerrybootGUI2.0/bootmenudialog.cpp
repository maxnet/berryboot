/* Berryboot -- boot menu dialog
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

#include "bootmenudialog.h"
#include "ui_bootmenudialog.h"
#include "installer.h"
#include "localedialog.h"
#include "diskdialog.h"
#include "adddialog.h"
#include "mainwindow.h"

#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/reboot.h>

#include <QFile>
#include <QProgressDialog>
#include <QProcess>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QTime>
#include <QDebug>
#include <QCloseEvent>

#define runonce_file  "/mnt/data/runonce"
#define default_file  "/mnt/data/default"


BootMenuDialog::BootMenuDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BootMenuDialog),
    _i(i),
    _countdown(11)
{
    ui->setupUi(this);
    setEnabled(false);
    QTimer::singleShot(1, this, SLOT(initialize()));
}

BootMenuDialog::~BootMenuDialog()
{
    stopSSHserver();
    delete ui;
}

void BootMenuDialog::closeEvent(QCloseEvent *ev)
{
    ev->ignore();
}

/* Mount data partition and populate menu
 * TODO: move stuff to seperate thread instead of messing with processEvents()
 */
void BootMenuDialog::initialize()
{
    bool success   = false;
    QByteArray datadev = _i->bootParam("datadev");
    QByteArray qmap    = _i->bootParam("qmap");

    if (!qmap.isEmpty())
        _i->setKeyboardLayout(qmap);

    startSSHserverIfEnabled();

    if (!datadev.isEmpty())
    {
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

    QProgressDialog qpd(tr("Waiting for data partition %1").arg(QString(datadev)), QString(), 0, 0, this);
    qpd.show();
    QApplication::processEvents();

    /* Wait 10 seconds for named data partition */
    waitForDevice(datadev);

    qpd.setLabelText(tr("Mounting data partition %1").arg(QString(datadev)));
    QApplication::processEvents();

    if (_i->bootoptions().contains("luks"))
    {
        askLuksPassword(datadev);
        success = mountDataPartition("mapper/luks");
    }
    else
    {
        success = mountDataPartition(datadev);
    }

    if (!success)
    {
        qpd.setLabelText(tr("Data is not at %1. Searching other partitions...").arg(QString(datadev)));
        QApplication::processEvents();
        datadev.clear();

        /* Search 10 times */
        for (unsigned int i=0; i<10 && datadev.isEmpty(); i++)
        {
            if (i != 0)
            {
                processEventSleep(1000);
            }

            datadev = getPartitionByLabel();
        }

        if (!datadev.isEmpty())
        {
            qpd.setLabelText(tr("Found other berryboot partition to mount: %1").arg(QString(datadev)));
            QApplication::processEvents();
            success = mountDataPartition(datadev);
        }
    }

    if (!success)
    {
        qpd.hide();

        datadev = _i->bootParam("datadev");
        if (_i->bootoptions().contains("luks"))
            datadev = "mapper/luks";

        if (QFile::exists("/dev/"+datadev) && !_i->bootoptions().contains("btrfs"))
        {
            if (QMessageBox::question(this, tr("Perform fsck?"),
                tr("Error mounting data partition. Try to repair file system?"), QMessageBox::Yes, QMessageBox::No)
                    == QMessageBox::Yes)
            {
                QProcess proc;
                _i->switchConsole(5);
                proc.start(QByteArray("openvt -c 5 -w /usr/sbin/fsck.ext4 -yf /dev/"+datadev));
                QApplication::processEvents();
                proc.waitForFinished();
                success = mountDataPartition(datadev);
                _i->switchConsole(1);
            }
        }

        if (!success)
        {
            QMessageBox::critical(this, tr("No data found..."), tr("Cannot find my data partition :-("), QMessageBox::Ok);
            reject();
            return;
        }
    }
    initializeA10();

    if (QFile::exists(runonce_file))
    {
        waitForRemountRW();
        qpd.setLabelText(tr("Removing runonce file"));
        QApplication::processEvents();
        QByteArray runonce = file_get_contents(runonce_file);
        QFile::remove(runonce_file);
        sync();
        file_put_contents("/tmp/answer", runonce);
        reject();
        return;
    }

    QString def = _i->getDefaultImage();
    QMap<QString,QString> images = _i->listInstalledImages();
    if (images.isEmpty())
    {
        startInstaller();
        return;
    }


    if (def.isEmpty() )
        def = images.begin().key();

    for (QMap<QString,QString>::const_iterator iter = images.constBegin(); iter != images.constEnd(); iter++)
    {
        QListWidgetItem *item = new QListWidgetItem(QIcon(), iter.value(), ui->list);
        item->setData(Qt::UserRole, iter.key());
        if (def == iter.key())
            ui->list->setCurrentItem(item);
    }

    if (_i->bootoptions().contains("sound"))
    {
        /* Set sound channel (HDMI/headphones) */
        _i->loadSoundModule(_i->bootParam("sound"));
    }

    if (!_i->bootoptions().contains("nobootmenutimeout"))
    {
        // start timer

        QByteArray bootmenutimeout = _i->bootParam("bootmenutimeout");
        if (!bootmenutimeout.isEmpty())
        {
            _countdown = bootmenutimeout.toInt()+1;
        }

        autoBootTimeout();
        connect(&_countdownTimer, SIGNAL(timeout()), this, SLOT(autoBootTimeout()));
        _countdownTimer.setInterval(1000);
        _countdownTimer.start();
        ui->list->installEventFilter(this);
        connect(ui->list, SIGNAL(itemPressed(QListWidgetItem*)), this, SLOT(stopCountdown()));
    }
    setEnabled(true);
    ui->list->setFocus();
}

void BootMenuDialog::on_bootButton_clicked()
{
    bootImage(ui->list->currentItem()->data(Qt::UserRole).toString());
}

void BootMenuDialog::on_settingsButton_clicked()
{
    stopCountdown();
    startInstaller();
}

void BootMenuDialog::on_poweroffButton_clicked()
{
    halt();
}

void BootMenuDialog::on_list_activated(const QModelIndex &)
{
    on_bootButton_clicked();
}

void BootMenuDialog::autoBootTimeout()
{
    ui->bootButton->setText(tr("Boot (%1) ").arg(QString::number(--_countdown)));
    if (!_countdown)
    {
        _countdownTimer.stop();
        on_bootButton_clicked();
    }
}

/* Stop autoboot countdown on any key press or mouse button click */
bool BootMenuDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress || event->type() == QEvent::MouseButtonPress)
    {
        stopCountdown();

        if (obj == ui->list)
        {
            ui->list->removeEventFilter(this);
            return false;
        }
    }

    return QDialog::eventFilter(obj, event);
}

void BootMenuDialog::stopCountdown()
{
    if (_countdownTimer.isActive())
    {
        _countdownTimer.stop();
        ui->bootButton->setText(tr("Boot "));
    }
}

void BootMenuDialog::bootImage(const QString &name)
{
    int currentmemsplit = currentMemsplit();
    int needsmemsplit   = imageNeedsMemsplit(name);
    waitForRemountRW();

    if (_i->isMemsplitHandlingEnabled() && needsmemsplit && needsmemsplit != currentmemsplit)
    {
        mountSystemPartition();
        QProgressDialog qpd(tr("Changing memsplit and rebooting..."), QString(), 0, 0, this);
        qpd.show();
        QApplication::processEvents();

        /* Edit config.txt */
        QByteArray newconfig, oldconfig = file_get_contents("/boot/config.txt");
        QList<QByteArray> lines = oldconfig.split('\n');

        /* Copy all lines except the old gpu_mem= parameter to the new config.txt */
        foreach (QByteArray line, lines)
        {
            if (!line.startsWith("gpu_mem="))
                newconfig += line + "\n";
        }
        newconfig = newconfig.trimmed()+"\n";
        newconfig += memsplitParameter(needsmemsplit);
        /* write new config.txt to temporary file first */
        file_put_contents("/boot/config.new", newconfig);
        /* sync() the data of the new file to disk */
        sync();
        /* rename() it. This is an atomic operation according to man page */
        rename("/boot/config.new", "/boot/config.txt");
        umountSystemPartition();
        file_put_contents(runonce_file, name.toAscii());
        QProcess::execute("umount /mnt");
        sync(); //sleep(1);
        reboot();
    }
    else
    {
        file_put_contents("/tmp/answer", name.toAscii() );
        reject();
    }
}

bool BootMenuDialog::memsplitsEnabled()
{
    /* CCM for the win */
    return false;
}

void BootMenuDialog::startInstaller()
{
    startNetworking();
    mountSystemPartition();
    waitForRemountRW();
    accept();
}

void BootMenuDialog::startISCSI()
{
    loadModule("iscsi_tcp");
    startNetworking();

    QProgressDialog qpd(tr("Waiting for network to be ready"), QString(), 0, 0, this);
    qpd.show();
    QApplication::processEvents();

    for (unsigned int i=0; i<10 && !_i->networkReady(); i++)
    {
            processEventSleep(500);
    }

    qpd.setLabelText(tr("Connecting to iSCSI SAN"));
    QApplication::processEvents();

    mountSystemPartition();
    if (system("sh /boot/iscsi.sh 2>/dev/null") != 0)
    {
        QMessageBox::critical(this, tr("iSCSI error"), tr("Error connecting to server"), QMessageBox::Ok);
    }
    if (symlink("/dev/sda1", "/dev/iscsi"))
    {
        // show error?
    }
    umountSystemPartition();
}

void BootMenuDialog::loadModule(const QByteArray &name)
{
    QProgressDialog qpd(QString(), QString(), 0, 0, this);

    if (!QFile::exists("/lib/modules"))
    {
        if (!QFile::exists("/mnt/shared"))
        {
            /* Mount boot partition, and uncompress /boot/shared.tgz to memory */
            mountSystemPartition();
            qpd.setLabelText(tr("Uncompressing drivers"));
            qpd.show();
            QApplication::processEvents();
            _i->prepareDrivers();
            umountSystemPartition();
        }
        else
        {
            /* Use modules from /mnt/shared on disk */
            _i->prepareDrivers();
        }
    }

    qpd.setLabelText(tr("Loading module: %1").arg(QString(name)));
    qpd.show();
    QApplication::processEvents();
    QProcess::execute("/sbin/modprobe", QStringList(name));
}

void BootMenuDialog::initializeA10()
{
    QByteArray cpuinfo = file_get_contents("/proc/cpuinfo");

    if (cpuinfo.contains("sun4i") || cpuinfo.contains("sun5i"))
    {
        /* Some Allwinner A10/A13 drivers are not compiled into the kernel
           load them as module
           FIXME: driver should be loaded dynamically
         */
        /*loadModule("ump");
        loadModule("mali");
        loadModule("mali_drm");*/
        // Wifi
        loadModule("8192cu");
    }

    /* If using a static wifi configuration, start it now */
    if (_i->bootParam("ipv4").endsWith("/wlan0"))
    {
        mountSystemPartition();
        _i->startWifi();
        umountSystemPartition();
    }
}


/* Utility functions copied from old BerrybootUI application */
QByteArray BootMenuDialog::file_get_contents(const QString &filename)
{
    QFile f(filename);
    f.open(f.ReadOnly);
    QByteArray data = f.readAll();
    f.close();
    return data;
}

void BootMenuDialog::file_put_contents(const QString &filename, const QByteArray &data)
{
    QFile f(filename);
    f.open(f.WriteOnly);
    f.write(data);
    f.close();
}

QByteArray BootMenuDialog::getBootOptions()
{
    return _i->bootoptions();
}

bool BootMenuDialog::mountDataPartition(const QString &dev)
{
    QString mountoptions = "-o noatime,ro";

    if (!QFile::exists("/mnt"))
        mkdir("/mnt", 0755);

    if (getBootOptions().contains("fstype=btrfs"))
    {
        mountoptions += ",compress=lzo -t btrfs";
        loadModule("btrfs");
    }
    else
    {
        mountoptions += " -t ext4";
    }

    /* Try mounting read-only first.
     * If that fails try mounting read-write straight away as it might recover from journal */
    if (QProcess::execute("mount "+mountoptions+" /dev/"+dev+" /mnt") != 0
        && QProcess::execute("mount "+mountoptions.replace(",ro","")+" /dev/"+dev+" /mnt") != 0)
    {
        return false;
    }

    if (!QFile::exists("/mnt/images"))
    {
        if (QProcess::execute("umount /mnt") != 0)
        {
            //log_error("Error unmounting data partition");
        }
        return false;
    }

    /* Remount read-write in the background */
    _remountproc.start("mount -o remount,rw /mnt");

    return true;
}

void BootMenuDialog::waitForRemountRW()
{
    if (_remountproc.state() != _remountproc.NotRunning)
    {
        QProgressDialog qpd(tr("Remounting data partition read-write"), QString(), 0, 0, this);
        qpd.show();
        QApplication::processEvents();

        if (!_remountproc.waitForFinished())
        {
            QMessageBox::critical(this, tr("Error remounting data partition"), tr("Timed out waiting for remounting data partition read-write to complete"), QMessageBox::Ok);
        }
    }
}

bool BootMenuDialog::waitForDevice(const QString &dev)
{
    QTime t;
    t.start();

    while (t.elapsed() < 10000)
    {
        if (QFile::exists("/dev/"+dev))
            return true;

        QApplication::processEvents(QEventLoop::WaitForMoreEvents, 250);
    }

    return false;
}

QByteArray BootMenuDialog::getPartitionByLabel(const QString &label)
{
    QByteArray dev;
    QProcess proc;
    proc.start("/sbin/findfs LABEL="+label);
    if (proc.waitForFinished() && proc.exitCode() == 0)
    {
        dev = proc.readAll().trimmed();
        if (dev.size()>5) /* Remove /dev and trailing \n */
            dev = dev.mid(5);
    }

    return dev;
}

void BootMenuDialog::mountSystemPartition()
{
    QProgressDialog qpd(tr("Mounting system partition..."), QString(), 0, 0, this);
    qpd.show();
    QApplication::processEvents();

    if (!QFile::exists("/boot"))
        mkdir("/boot", 0755);

    waitForDevice("mmcblk0");

    for (unsigned int tries=0; tries<2; tries++)
    {
        if (QProcess::execute("mount /dev/mmcblk0p1 /boot") == 0 || QProcess::execute("mount /dev/mmcblk0 /boot") == 0)
            return;

        qpd.setLabelText(tr("Error mounting system partition... Retrying..."));
        processEventSleep(1000);
    }

    qpd.hide();
    QMessageBox::critical(this, tr("Error mounting system partition"), tr("Unable to mount system partition"), QMessageBox::Ok);
}

void BootMenuDialog::startNetworking()
{
    bool eth0available = false;

    QProgressDialog qpd(tr("Waiting for network device (eth0)"), QString(), 0, 0, this);
    qpd.show();
    QApplication::processEvents();

    for (unsigned int i=0; i<10 && !eth0available; i++)
    {
        if (i != 0)
            processEventSleep(500);

        eth0available = QFile::exists("/sys/class/net/eth0");
    }
    if (!eth0available)
        return;

    QProcess *proc = new QProcess();
    connect(proc, SIGNAL(finished(int)), proc, SLOT(deleteLater()));
    proc->start("sh -c \"/sbin/ifup eth0 || /sbin/ifup eth0\"");
}

void BootMenuDialog::umountSystemPartition()
{
    QProcess::execute("umount /boot");
}

int BootMenuDialog::currentMemsplit()
{
    QByteArray meminfo = file_get_contents("/proc/meminfo");
    //  MemTotal:        3713768 kB

    QByteArray memtotalstr = meminfo.mid(0, meminfo.indexOf(" kB"));
    memtotalstr = memtotalstr.mid(memtotalstr.lastIndexOf(" ")+1);
    int memtotal = memtotalstr.toInt();

    if (memtotal > 496000) /* 512 MB model. Return corespondening 256 MB model values for compatibility */
        return 240;
    else if (memtotal > 480000)
        return 224;
    else if (memtotal > 448000)
        return 192;
    else if (memtotal > 256000)
        return 128;
    else if (memtotal > 230000) /* Original 256 MB model */
        return 240;
    else if (memtotal > 200000)
        return 224;
    else if (memtotal > 130000)
        return 192;
    else
        return 128;
}

int BootMenuDialog::imageNeedsMemsplit(const QString &name)
{
    /* image file names are like: berryterminal.img224 */

    int imgpos = name.lastIndexOf(".img");
    if (imgpos != -1 && imgpos+5 < name.size())
    {
        return name.mid(imgpos+4).toInt();
    }
    else
    {
        return 0;
    }
}

QByteArray BootMenuDialog::memsplitParameter(int memsplit)
{
    const char *r;

    switch (memsplit)
    {
        case 240:
            r = "gpu_mem=16\n";
            break;
        case 224:
            r = "gpu_mem=32\n";
            break;
        case 128:
            r = "gpu_mem=128\n";
            break;
        default:
            r = "gpu_mem=64\n";
    }

    return r;
}

void BootMenuDialog::reboot()
{
    QProcess::execute("umount -ar");
    sync();
    ::reboot(RB_AUTOBOOT);
}

void BootMenuDialog::halt()
{
    QProcess::execute("umount -ar");
    sync();
    ::reboot(RB_POWER_OFF);
}

void BootMenuDialog::processEventSleep(int ms)
{
    QTime t;
    t.start();

    while (t.elapsed() < ms)
    {
        int sleepfor = ms-t.elapsed();
        if (sleepfor > 0)
            QApplication::processEvents(QEventLoop::WaitForMoreEvents, sleepfor);
    }
}

void BootMenuDialog::askLuksPassword(const QString &datadev)
{
    loadModule("dm_crypt");
    loadModule("aes");
    loadModule("sha256");
    loadModule("algif_hash");

    /* For added security let cryptsetup ask for password in a text console,
     * as it can remain in memory if we do it in the GUI.
     * Also allow it to be entered through SSH if enabled.
     */
    QProcess proc;
    ::mkdir("/etc/profile.d", 0755);
    QFile f("/etc/profile.d/ssh-luks-prompt.sh");
    f.open(f.WriteOnly);
    f.write("/sbin/cryptsetup.static luksOpen /dev/"+datadev.toAscii()+" luks && killall openvt");
    f.close();

    _i->switchConsole(5);
    while (!QFile::exists("/dev/mapper/luks"))
    {
        proc.start(QByteArray("openvt -c 5 -w /sbin/cryptsetup.static luksOpen /dev/")+datadev+" luks");
        QApplication::processEvents();
        proc.waitForFinished();
    }

    _i->switchConsole(1);
    QFile::remove("/etc/profile.d/ssh-luks-prompt.sh");
}

void BootMenuDialog::startSSHserverIfEnabled()
{
    QByteArray sshkey = _i->bootParam("ssh_authorized_key");
    if (sshkey.isEmpty())
        return;

    /* Write authorized_key provided through boot parameter to tmpfs */
    ::chmod("/root", 0700);
    ::mkdir("/root/.ssh", 0700);
    QFile f("/root/.ssh/authorized_keys");
    f.open(f.WriteOnly);
    f.write(sshkey);
    f.close();

    /* Let dropbear store the host's public key on the FAT partition */
    mountSystemPartition();

    QProgressDialog qpd(tr("Starting SSH server\nCan take a while on first run..."), QString(), 0, 0, this);
    qpd.show();
    QApplication::processEvents();

    QFile::link("/boot", "/etc/dropbear");
    ::mkdir("/dev/pts", 0755);
    QProcess::execute("mount -t devpts devpts /dev/pts");
    QProcess::execute("/etc/init.d/S50dropbear start");

    umountSystemPartition();
}

void BootMenuDialog::stopSSHserver()
{
    if (!_i->bootParam("ssh_authorized_key").isEmpty())
    {
        QProcess::execute("/etc/init.d/S50dropbear stop");
        QProcess::execute("umount /dev/pts");
    }
}
