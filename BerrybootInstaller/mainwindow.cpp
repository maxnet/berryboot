/* Berryboot -- main boot menu editor window
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
#include "ui_mainwindow.h"
#include "installer.h"
#include "adddialog.h"
#include "editdialog.h"
#include "clonedialog.h"
#include "exportdialog.h"
#include "confeditdialog.h"
#include "copythread.h"

#include <QDateTime>
#include <QMenu>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QProgressDialog>
#include <QFileDialog>
#include <QListWidgetItem>

#include <unistd.h>

MainWindow::MainWindow(Installer *i, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _i(i)
{
    ui->setupUi(this);

    /* Add OS submenu */
    QMenu *menu = new QMenu(this);
    menu->addAction(QIcon(":/icons/server.png"),tr("Download OS from Internet"), this, SLOT(on_actionAdd_OS_triggered()));
    menu->addAction(QIcon(":/icons/usb.png"),tr("Copy OS from USB stick"), this, SLOT(copyOSfromUSB()));

    ui->actionAdd_OS->setMenu(menu);

    populate();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::closeEvent(QCloseEvent *)
{
    _i->reboot();
}

void MainWindow::populate()
{
    QString def = _i->getDefaultImage();
    QMap<QString,QString> images = _i->listInstalledImages();
    ui->list->clear();

    for (QMap<QString,QString>::const_iterator iter = images.constBegin(); iter != images.constEnd(); iter++)
    {
        QIcon icon;
        if (iter.key() == def)
            icon = QIcon(":/icons/bookmark.png");

        QListWidgetItem *item = new QListWidgetItem(icon, iter.value(), ui->list);
        item->setData(Qt::UserRole, iter.key());
    }

    ui->statusBar->showMessage(tr("Disk: %1 MB available").arg( QString::number((int) (_i->availableDiskSpace()/1024/1024)) ) );
}

void MainWindow::on_actionAdd_OS_triggered()
{
    AddDialog a(_i);
    a.exec();
    populate();
}

void MainWindow::copyOSfromUSB()
{
    if (!scanUSBdevices())
        return;

    /* Prompt for image file */
    QString defaultdir = "/media";
    if (partlist.count() == 1)
        defaultdir += "/"+partlist.first();

    QString fileName = QFileDialog::getOpenFileName(this, tr("Select image file"), defaultdir, tr("SquashFS images (*.img *.img128 *.img192 *.img224 *.img240)"));
    if (!fileName.isEmpty())
    {
        QFile f(fileName);
        QFileInfo fi(f);
        QString destfile = "/mnt/images/"+fi.fileName();
        destfile.replace(" ", "_");

        if ( (f.size()+(10*1024*1024)) > _i->availableDiskSpace() )
        {
            QMessageBox::critical(this, tr("Unable to copy"), tr("Insufficient disk space"));
        }
        else if ( !_i->isSquashFSimage(f) )
        {
            QMessageBox::critical(this, tr("Unable to copy"), tr("Image is not in SquashFS format"));
        }
        else if ( QFile::exists(destfile))
        {
            QMessageBox::critical(this, tr("Unable to copy"), tr("There already exists an image by that name"));
        }
        else
        {
            QProgressDialog *qpd = new QProgressDialog(tr("Copying file..."), QString(), 0,0,this);
            setEnabled(false);
            qpd->show();

            f.close();
            CopyThread *ct = new CopyThread(fileName, destfile, this);
            connect(ct, SIGNAL(finished()), qpd, SLOT(deleteLater()));
            connect(ct, SIGNAL(completed()), this, SLOT(cleanupUSBdevices()));
            connect(ct, SIGNAL(failed()), this, SLOT(onCopyFailed()));
            ct->start();

            setEnabled(true);
            return;
        }
    }

    cleanupUSBdevices();
}

void MainWindow::onCopyFailed()
{
    QMessageBox::critical(this, tr("Unable to copy"), tr("Error copying file"));
}

bool MainWindow::scanUSBdevices(bool mountrw)
{
    /* Scan for storage devices */
    QString dirname  = "/sys/class/block";
    QDir    dir(dirname);
    QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (!dir.exists("/media"))
        dir.mkdir("/media");

    QProgressDialog qpd(tr("Scanning storage devices..."), QString(), 0, 0, this);
    setEnabled(false);
    qpd.show();
    QApplication::processEvents(); // risk of recursion?

    foreach (QString devname, list)
    {
        if (!devname.startsWith("mmcblk0") && !QFile::symLinkTarget("/sys/class/block/"+devname).contains("/devices/virtual/")  /*&& QFile::exists(dirname+"/"+devname+"/partition")*/ )
        {
            QString mntdir = "/media/"+devname;
            dir.mkdir(mntdir);
            QString cmd;

            if (mountrw)
                cmd = "mount /dev/"+devname+" "+mntdir;
            else
                cmd = "mount -o ro /dev/"+devname+" "+mntdir;

            if ( QProcess::execute(cmd) == 0 )
                partlist.append(devname);
            else
                dir.rmdir(mntdir);
        }
    }
    qpd.hide();
    setEnabled(true);

    if (partlist.empty())
    {
        QMessageBox::information(this, tr("No media found"), tr("Insert a USB stick or other external medium first, and try again."), QMessageBox::Close);
        return false;
    }

    return true;
}

void MainWindow::cleanupUSBdevices()
{
    QDir dir;

    /* Clean up */
    foreach (QString devname, partlist)
    {
        QProcess::execute("umount /media/"+devname);
        dir.rmdir("/media/"+devname);
    }

    populate();
}


void MainWindow::on_actionEdit_triggered()
{
    QString currentname = ui->list->currentItem()->data(Qt::UserRole).toString();

    EditDialog ed(currentname, this);
    if ( ed.exec() == QDialog::Accepted)
    {
        QString newname = ed.filename();

        if (newname != currentname && !newname.isEmpty())
        {
            if (QFile::exists("/mnt/image/"+newname) || QFile::exists("/mnt/data/"+newname))
            {
                QMessageBox::critical(this, tr("Invalid name"), tr("There already exists another image by that name"), QMessageBox::Close);
                return;
            }

            _i->renameImage(currentname, newname);
            setButtonsEnabled(false);
            populate();
        }
    }
}

void MainWindow::on_actionClone_triggered()
{
    QString currentname = ui->list->currentItem()->data(Qt::UserRole).toString();

    CloneDialog cd(this);
    if ( cd.exec() == QDialog::Accepted && !cd.filename().isEmpty())
    {
        QProgressDialog qpd(tr("Cloning image"),QString(),0,0,this);
        qpd.show();
        QApplication::processEvents();

        QString newname = cd.filename();
        int pos = currentname.lastIndexOf(".img");
        if (pos != -1)
            newname += currentname.mid(pos);

        _i->cloneImage(currentname, newname, cd.cloneData());

        setButtonsEnabled(false);
        populate();
    }
}

void MainWindow::on_actionDelete_triggered()
{
    if (QMessageBox::question(this, tr("Confirm deletion"), tr("Are you sure you want to delete '%1'").arg(ui->list->currentItem()->text() ), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        QProgressDialog qpd(tr("Deleting image"),QString(),0,0,this);
        qpd.show();
        QApplication::processEvents();

        _i->deleteImage( ui->list->currentItem()->data(Qt::UserRole).toString() );
        setButtonsEnabled(false);
        populate();
    }
}

void MainWindow::on_actionSet_default_triggered()
{
    int row = ui->list->currentRow();
    _i->setDefaultImage( ui->list->currentItem()->data(Qt::UserRole).toString() );
    populate();
    ui->list->setCurrentRow(row);
}

void MainWindow::on_list_currentRowChanged(int)
{
    setButtonsEnabled(true);
}

void MainWindow::setButtonsEnabled(bool enable)
{
    ui->actionEdit->setEnabled(enable);
    ui->actionClone->setEnabled(enable);
    ui->actionDelete->setEnabled(enable);
    ui->actionSet_default->setEnabled(enable);
    ui->actionExport->setEnabled(enable);
}

void MainWindow::on_actionExport_triggered()
{
    QString imagename = ui->list->currentItem()->data(Qt::UserRole).toString();

    ExportDialog ed(this);
    if (ed.exec() == QDialog::Accepted)
    {
        if (!scanUSBdevices(true) )
            return;

        /* Prompt for image file */
        QString defaultpath = "/media/"+partlist.first()+"/";

        if (ed.exportData())
        {
            /* Prefix user modified images with date */
            //defaultpath += QDateTime::currentDateTime().toString("yyyyMMdd")+"-";
            /* Not working without RTC. duh */
            defaultpath += "MODIFIED-";
        }

        defaultpath += imagename;

        QString fileName = QFileDialog::getSaveFileName(this, tr("Select image file"), defaultpath, tr("SquashFS images (*.img *.img128 *.img192 *.img224 *.img240)"));
        if (!fileName.isEmpty() && fileName.startsWith("/media/")) /* TODO: for possibility of symlinks? */
        {
            if (ed.exportData() && QFile::exists("/mnt/data/"+imagename))
            {
                mksquashfs(imagename, fileName, ed.excludeList());
            }
            else
            {
                QProgressDialog *qpd = new QProgressDialog(tr("Copying file..."), QString(), 0,0,this);
                qpd->show();
                CopyThread *ct = new CopyThread("/mnt/images/"+imagename, fileName, this);
                connect(ct, SIGNAL(finished()), qpd, SLOT(deleteLater()));
                connect(ct, SIGNAL(completed()), this, SLOT(cleanupUSBdevices()));
                connect(ct, SIGNAL(failed()), this, SLOT(onCopyFailed()));
                ct->start();
            }

            return;
        }

        cleanupUSBdevices();
    }
}

void MainWindow::mksquashfs(QString imagename, QString destfileName, QStringList exclList)
{
    QProgressDialog *qpd = new QProgressDialog(tr("Mounting image..."), QString(),0,0,this);
    qpd->show();
    QApplication::processEvents();

    QDir dir;
    dir.mkdir("/squashfs");
    dir.mkdir("/aufs");

    if ( QProcess::execute("/bin/mount -o loop \"/mnt/images/"+imagename+"\" /squashfs") != 0)
    {
        qpd->deleteLater();
        QMessageBox::critical(this, tr("mksquashfs error"), tr("Error mounting original image"));
        cleanupUSBdevices();
        return;
    }


    if (QProcess::execute("/bin/mount -t aufs -o br:/mnt/data/"+imagename+":/squashfs none /aufs") != 0)
    {
        qpd->deleteLater();
        QMessageBox::critical(this, tr("mksquashfs error"), tr("Error mounting data dir on top"));
        QProcess::execute("/bin/umount /squashfs");
        cleanupUSBdevices();
        return;
    }


    QStringList args;
    args << "/aufs" << destfileName << "-comp" << "lzo" << "-noF" << "-noD" << "-no-progress" << "-read-queue" << "16" << "-write-queue" << "16" << "-fragment-queue" << "16";
    foreach (QString exclude, exclList)
    {
        if (exclude.startsWith("/"))
            exclude = exclude.mid(1);
        if (!exclude.isEmpty())
            args << "-e" << exclude;
    }

    QProcess *proc = new QProcess();
    proc->setProcessChannelMode(proc->MergedChannels);
    connect(proc, SIGNAL(finished(int)), this, SLOT(mksquashfsFinished(int)));
    connect(proc, SIGNAL(finished(int)), qpd, SLOT(deleteLater()));

    qpd->setLabelText(tr("Exporting your image... Can take half an hour."));
    proc->start("/usr/bin/mksquashfs", args);
}

void MainWindow::mksquashfsFinished(int code)
{
    QProcess *proc = qobject_cast<QProcess *>(sender());

    QProcess::execute("/bin/umount /aufs");
    QProcess::execute("/bin/umount /squashfs");
    sync();
    cleanupUSBdevices();

    if (code != 0)
    {
        QMessageBox::critical(this, tr("mksquashfs error"), tr("Error creating image.\n\n%1").arg( QString(proc->readAll()) ));
    }

    proc->deleteLater();
}

void MainWindow::on_actionAdvanced_configuration_triggered()
{
    ConfEditDialog d;
    d.exec();
}
