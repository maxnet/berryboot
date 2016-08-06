/* Berryboot -- download dialog
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

#include "downloaddialog.h"
#include "ui_downloaddialog.h"
#include "syncthread.h"
#include "downloadthread.h"
#include <QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkProxy>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTimer>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <QDebug>
#include <QCloseEvent>


#define MB  1048576

DownloadDialog::DownloadDialog(const QString &url, const QString &alternateUrl, const QString &localfilename, Filetype fileType, const QString &sha1, QWidget *parent):
    QDialog(parent),
    ui(new Ui::DownloadDialog),
    _hasher(QCryptographicHash::Sha1),
    _expectedHash(sha1),
    _localfilename(localfilename),
    _alternateUrl(alternateUrl),
    _fileType(fileType),
    _100kbdownloaded(0), _100kbtotal(0)
{
    ui->setupUi(this);

    if (fileType == Image && QFile::exists("/mnt/images/"+localfilename))
    {
        QMessageBox::critical(NULL, tr("Already installed"), tr("You already have an image by that name.\nPress 'more options' -> 'clone' if you want a second instance"), QMessageBox::Close);
        QTimer::singleShot(1, this, SLOT(reject()));
        return;
    }

    _download = new DownloadThread(url.toLatin1(), "/mnt/tmp/"+localfilename, this);
    connect(_download, SIGNAL(downloadSuccessful()), this, SLOT(onDownloadSuccessful()));
    connect(_download, SIGNAL(downloadError(QString)), this, SLOT(onDownloadError(QString)));
    connect(_download, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));

    _time.start();
    _download->start();
}

DownloadDialog::~DownloadDialog()
{
    delete ui;
}

void DownloadDialog::onDownloadError(const QString &message)
{
    QMessageBox::critical(this, tr("Download error"), tr("Error downloading file from Internet: ")+message, QMessageBox::Close);

    if (!_alternateUrl.isEmpty())
    {
        if (QMessageBox::question(this, tr("Try other mirror?"), tr("Would you like to retry downloading from a different site?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
        {
            _download = new DownloadThread(_alternateUrl.toLatin1(), "/mnt/tmp/"+_localfilename, this);
            connect(_download, SIGNAL(downloadSuccessful()), this, SLOT(onDownloadSuccessful()));
            connect(_download, SIGNAL(downloadError(QString)), this, SLOT(onDownloadError(QString)));
            connect(_download, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
            _time.start();
            _download->start();
            _100kbdownloaded = _100kbtotal = 0;
            _alternateUrl.clear();
            return;
        }
    }

    reject();
}

void DownloadDialog::onDownloadSuccessful()
{
    if (!_expectedHash.isEmpty() && _expectedHash != _download->sha1())
    {
        _download->deleteDownloadedFile();
        onDownloadError(tr("file corrupt (sha1 does not match)"));
        return;
    }
    //qDebug() << "Hash expected:" << _expectedHash << "Hash calculated:" << _hasher.result().toHex();

    QByteArray filename = "/mnt/tmp/"+_localfilename.toLatin1();
    for (QMap<QByteArray,QByteArray>::const_iterator iter = _xattr.constBegin(); iter != _xattr.constEnd(); iter++)
    {
        ::setxattr(filename.constData(), iter.key().constData(), iter.value().constData(), iter.value().length(), 0);
    }

    QProgressDialog *qpd = new QProgressDialog(tr("Finish writing to disk (sync)"), QString(),0,0,this);
    qpd->show();

    SyncThread *st = new SyncThread(this);
    connect(st, SIGNAL(finished()), qpd, SLOT(hide()));
    connect(st, SIGNAL(finished()), SLOT(onSyncComplete()));
    st->start();
}

void DownloadDialog::onSyncComplete()
{
    if (_fileType == Image)
    {
        QFile::rename("/mnt/tmp/"+_localfilename, "/mnt/images/"+_localfilename);
        sync();
    }
    accept();
}

void DownloadDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (!_100kbtotal && bytesReceived > 4)
    {
        _100kbtotal = bytesTotal*10/MB;
        ui->progressBar->setMaximum(_100kbtotal);
        // TODO: magic check
    }
    int b = bytesReceived*10/MB;

    if (b != _100kbdownloaded)
    {
        _100kbdownloaded = b;
        ui->bytesLabel->setText(tr("%1 MB of %2 MB").arg( QString::number(_100kbdownloaded/10.0,'f',1), _100kbtotal ? QString::number(_100kbtotal/10.0,'f',1) : "unknown"));

        int downloadRate = bytesReceived / qMax(_time.elapsed()/1000, 1);
        ui->speedLabel->setText(tr("%1 mbit").arg( QString::number(downloadRate*8.0/MB,'f', 1)));

        int etahr = 0, etamin = 0, etasec = (bytesTotal-bytesReceived)/downloadRate;
        if (etasec > 60)
            etamin = etasec/60;
        if (etamin > 60)
        {
            etahr = etamin/60;
            etamin = etamin % 60;
        }
        ui->etaLabel->setText(tr("%1 h %2 m").arg(QString::number(etahr), QString::number(etamin)));
        ui->progressBar->setValue(_100kbdownloaded);
    }
}

/* Cancel download */
void DownloadDialog::closeEvent(QCloseEvent *ev)
{
    if (_download)
    {
        _download->cancelDownload();
    }

    QDialog::closeEvent(ev);
}

void DownloadDialog::setAttr(const QByteArray &key, QByteArray &value)
{
    _xattr.insert(key, value);
}
