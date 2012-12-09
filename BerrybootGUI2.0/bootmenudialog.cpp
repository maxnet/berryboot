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

#include <time.h>

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
    int pos;
    QByteArray datadev, options = getBootOptions();

    if ((pos = options.indexOf("datadev=")) != -1)
    {
        datadev = options.mid(pos+8);
        if ((pos = datadev.indexOf(" ")) != -1 || (pos = datadev.indexOf("\n")) != -1)
            datadev = datadev.mid(0, pos);

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
    success = mountDataPartition(datadev);

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
    initializeA10();

    if (!success)
    {
        qpd.hide();
        QMessageBox::critical(this, tr("No data found..."), tr("Cannot find my data partition :-("), QMessageBox::Ok);
        reject();
        return;
    }

    if (QFile::exists(runonce_file))
    {
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

    if (!options.contains("nobootmenutimeout"))
    {
        // start timer

        int pos = options.indexOf("bootmenutimeout=");
        if (pos != -1)
        {
            QByteArray s = options.mid(pos+16);
            if ((pos = s.indexOf(" ")) != -1 || (pos = s.indexOf("\n")) != -1)
                s = s.mid(0, pos);
            _countdown = s.toInt()+1;
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

    if (!cpuinfo.contains("sun4i") && !cpuinfo.contains("sun5i"))
        return; /* Not an Allwinner A10/A13, return */

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
    return file_get_contents("/proc/cmdline");
}

bool BootMenuDialog::mountDataPartition(const QString &dev)
{
    QString mountoptions = "-o noatime";

    if (!QFile::exists("/mnt"))
        mkdir("/mnt", 0755);

    if (getBootOptions().contains("fstype=btrfs"))
    {
        mountoptions += ",compress=lzo";
        loadModule("btrfs");
    }

    if (QProcess::execute("mount "+mountoptions+" /dev/"+dev+" /mnt") != 0)
        return false;

    if (!QFile::exists("/mnt/images"))
    {
        if (QProcess::execute("umount /mnt") != 0)
        {
            //log_error("Error unmounting data partition");
        }
        return false;
    }

    return true;
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
    proc->start("/sbin/ifup eth0");
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
