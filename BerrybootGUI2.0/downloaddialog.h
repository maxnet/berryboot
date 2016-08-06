#ifndef DOWNLOADDIALOG_H
#define DOWNLOADDIALOG_H

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

#include <QDialog>
#include <QCryptographicHash>
#include <QTime>
#include <QMap>

namespace Ui {
class DownloadDialog;
}
class QSettings;
class DownloadThread;
class QFile;

class DownloadDialog : public QDialog
{
    Q_OBJECT
    
public:
    enum Filetype { Image, Update, Other };

    /*
     * Constructor
     *
     * - url: URL to download
     * - alternateurl: URL to different mirror site, to try if downloading from first site fails
     * - localfilename: File name to save downloaded file as
     * - fileType: Image (operating system image), Update (Berryboot update) or Other
     * - sha1: SHA1 hash of file
     */
    explicit DownloadDialog(const QString &url, const QString &alternateUrl, const QString &localfilename, Filetype fileType, const QString &sha1 = "", QWidget *parent = NULL);
    void setAttr(const QByteArray &key, QByteArray &value);
    ~DownloadDialog();

protected:
    Ui::DownloadDialog *ui;
    QCryptographicHash _hasher;
    QString _expectedHash, _localfilename, _alternateUrl;
    DownloadThread *_download;
    QFile *_file;
    Filetype _fileType;
    QTime _time;
    QMap<QByteArray,QByteArray> _xattr;

    /*
     * Download progress (expressed in units of 100 kb)
     */
    int _100kbdownloaded, _100kbtotal;

    /*
     * Cancel download
     */
    virtual void closeEvent(QCloseEvent *ev);

protected slots:
    void onDownloadSuccessful();
    void onDownloadError(const QString &message);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onSyncComplete();
};

#endif // DOWNLOADDIALOG_H
