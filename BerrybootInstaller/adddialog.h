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
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkDiskCache;
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
    QNetworkAccessManager *_netaccess;
    QNetworkDiskCache *_cache;
    QString _cachedir;
    QNetworkReply *_reply;
    QSettings *_ini;

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

protected slots:
    void networkUp();
    void downloadComplete();
    void cancelDownload();

private slots:
    void on_osList_currentRowChanged(int currentRow);
    void onProxySettings();
};

#endif // ADDDIALOG_H
