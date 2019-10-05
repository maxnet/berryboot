#ifndef ADDDIALOG_H
#define ADDDIALOG_H

/* Berryboot -- add distribution dialog
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

namespace Ui {
class AddDialog;
}
class QProgressDialog;
class Installer;
class DownloadThread;
class QSettings;

class AddDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit AddDialog(Installer *i, QWidget *parent = 0);
    ~AddDialog();

public slots:
    void accept();
    
protected:
    Ui::AddDialog *ui;
    QProgressDialog *_qpd;
    Installer *_i;
    QString _cachedir, _device, _kernelversion, _prefmirror;
    QSettings *_ini;
    QByteArray _reposerver, _repouser, _repopass, _reposerver2;
    DownloadThread *_download, *_download2;
    bool _downloadCancelled;

    static bool _openSSLinitialized;
    static QByteArray _data;

    /*
     * Download distribution list from Internet
     */
    void downloadList();

    /*
     * Parse distribution list, and fill GUI listwidget
     */
    void processData();
    void processIni();

    /*
     * Generate the list from files in a CIFS or NFS network share
     */
    void generateListFromShare(const QByteArray &share, QByteArray username = "", QByteArray password = "");

    /*
     * Return SHA1 hash of file
     */
    QByteArray sha1file(const QString &filename);

    /*
     * Perform a Berryboot update
     */
    void selfUpdate(const QString &updateurl, const QString &sha1);

    /*
     * Returns true if signature matches
     */
    bool verifyData();

    /*
     * Set proxy settings from berryboot.ini
     */
    void setProxy();

    QByteArray getXattr(const QByteArray &filename, const QByteArray &key);

protected slots:
    void networkUp();
    void downloadComplete();
    void download2Complete();
    void cancelDownload();
    void generatePreloadedTab();

private slots:
    void onSelectionChanged();
    void onProxySettings();
    void on_groupTabs_currentChanged(int index);
};

#endif // ADDDIALOG_H
