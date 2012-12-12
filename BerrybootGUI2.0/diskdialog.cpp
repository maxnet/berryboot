/*
Copyright (c) 2012, Floris Bos
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "diskdialog.h"
#include "ui_diskdialog.h"
#include "installer.h"
#include "syncthread.h"
#include "driveformatthread.h"
#include "iscsidialog.h"
#include <QDir>
#include <QIcon>
#include <QProgressDialog>
#include <QMessageBox>
#include <unistd.h>

DiskDialog::DiskDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DiskDialog),
    _i(i)
{
    ui->setupUi(this);
    connect(&_pollDisksTimer,SIGNAL(timeout()), this, SLOT(pollForNewDisks()));
    connect(i, SIGNAL(error(QString)), this, SLOT(onError(QString)));

    populateDrivelist();
    /* Check every second if the user attached a USB disk */
    _pollDisksTimer.start(1000);
}

DiskDialog::~DiskDialog()
{
    delete ui;
}

inline QByteArray get_file_contents(const QString &filename)
{
    QByteArray r;

    QFile f(filename);
    f.open(f.ReadOnly);
    r = f.readAll();
    f.close();

    return r;
}

void DiskDialog::populateDrivelist()
{
    ui->driveList->clear();

    QString dirname  = "/sys/class/block";
    QDir    dir(dirname);
    QStringList list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (QString devname, list)
    {
        QString blocklink = QFile::symLinkTarget(dirname+"/"+devname);
        /* skip virtual things and partitions */
        if (blocklink.contains("/devices/virtual/") || QFile::exists(blocklink+"/partition") )
            continue;

        if (get_file_contents(blocklink+"/ro").trimmed() == "0")
        {
            /* block device is read-write */
            QString model = get_file_contents(dirname+"/"+devname+"/device/model").trimmed();
            if (model.isEmpty())
                model = get_file_contents(dirname+"/"+devname+"/device/name").trimmed();
            QIcon icon;

            if (devname.startsWith("mmc"))
            {
                icon = QIcon(":/icons/mmc.png");
            }
            else
            {
                icon = QIcon(":/icons/hdd.png");
            }
            QListWidgetItem *item = new QListWidgetItem(icon, devname+": "+model, ui->driveList);
            item->setData(Qt::UserRole, devname);
        }
    }
    QListWidgetItem *item = new QListWidgetItem(QIcon(":/icons/server.png"), "Networked storage (iSCSI SAN)", ui->driveList);
    item->setData(Qt::UserRole, "iscsi");


    ui->driveList->setCurrentRow(0);

    _devlistcount = list.count();
}

void DiskDialog::pollForNewDisks()
{
    QDir dir("/sys/class/block");
    if (dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).count() != _devlistcount)
        populateDrivelist();
}

void DiskDialog::on_formatButton_clicked()
{
    setEnabled(false);
    _pollDisksTimer.stop();
    QString drive = ui->driveList->currentItem()->data(Qt::UserRole).toString();
    QString fs;

    if (drive == "iscsi")
    {
        iSCSIDialog id(_i, this);
        if (id.exec() != id.Accepted)
        {
            setEnabled(true);
            _pollDisksTimer.start(1000);
            return;
        }
    }

    switch (ui->filesystemCombo->currentIndex())
    {
/*    case 0:
        fs = "btrfs";
        break; */
    case 0 /*1*/:
        fs = "ext4";
        break;
    default:
        fs = "ext4 nodiscard";
    }

    if (ui->luksCheck->isChecked())
    {
        QMessageBox::information(this, tr("Info"), tr("You will be switched to a secure text console shortly, where you need to enter the password you wish to use three times"), QMessageBox::Ok);
    }

    _qpd = new QProgressDialog( tr("Preparing disk format"), QString(), 0, 0, this);
    _qpd->show();
    //_i->mountSystemPartition() // already done by berryboot


    DriveFormatThread *dft = new DriveFormatThread(drive, fs, _i, this, "mmcblk0p1", true, ui->luksCheck->isChecked());
    connect(dft, SIGNAL(statusUpdate(QString)), _qpd, SLOT(setLabelText(QString)));
    connect(dft, SIGNAL(error(QString)), this, SLOT(onError(QString)));
    connect(dft, SIGNAL(completed()), this, SLOT(onFormatComplete()));
    dft->start();
}

void DiskDialog::onFormatComplete()
{
    DriveFormatThread *dft = qobject_cast<DriveFormatThread *>(sender());

    _qpd->hide();
    dft->deleteLater();
    accept();
}

void DiskDialog::onError(const QString &errormsg)
{
    if (_qpd)
        _qpd->hide();
    QMessageBox::critical(this, tr("Error"), errormsg, QMessageBox::Close);
    setEnabled(true);
    _pollDisksTimer.start(1000);
}
