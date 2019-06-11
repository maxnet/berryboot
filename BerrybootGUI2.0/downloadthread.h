#ifndef DOWNLOADTHREAD_H
#define DOWNLOADTHREAD_H

/* Berryboot -- download thread using libcurl
 *
 * Copyright (c) 2013, Floris Bos
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

#include <QThread>
#include <QCryptographicHash>
#include <time.h>
#include <curl/curl.h>

class QFile;

class DownloadThread : public QThread
{
    Q_OBJECT
public:

    /*
     * Constructor
     *
     * - url: URL to download
     * - localfilename: File name to save downloaded file as. If empty, store data in memory buffer
     */
    explicit DownloadThread(const QByteArray &url, const QString &localfilename = QString(), QObject *parent = 0);

    /*
     * Destructor
     *
     * Waits until download is complete
     * If this is not desired, call cancelDownload() first
     */
    virtual ~DownloadThread();

    /*
     * Cancel download
     *
     * Async function. Returns immedeately, but can take a second before download actually stops
     */
    void cancelDownload();

    /*
     * Set proxy server.
     * Specify a string like this: user:pass@proxyserver:8080/
     * Used globally, for all connections
     */
    static void setProxy(const QByteArray &proxy);

    /*
     * Returns proxy server used
     */
    static QByteArray proxy();

    /*
     * Set user-agent header string
     */
    void setUserAgent(const QByteArray &ua);

    /*
     * Returns true if download has been successful
     */
    bool successfull();

    /*
     * Returns the downloaded data if saved to memory buffer instead of file
     */
    QByteArray data();

    /*
     * Returns SHA1 hash of downloaded data
     * Do not call until download finishes
     */
    QByteArray sha1();

    /*
     * Delete downloaded file
     */
    void deleteDownloadedFile();

    /*
     * Return last-modified date (if available) as unix timestamp
     * (seconds since 1970)
     */
    time_t lastModified();

    /*
     * Return current server time as unix timestamp
     */
    time_t serverTime();

    void setCacheDirectory(const QString &dir);

    /*
     * libcurl callbacks
     */
    size_t _writeData(const char *buf, size_t len);
    bool _progress(curl_off_t dltotal, curl_off_t  dlnow, curl_off_t  ultotal, curl_off_t  ulnow);
    void _header(QByteArray &header);

protected:
    virtual void run();

    CURL *_c;
    curl_off_t _lastDlTotal, _lastDlNow, _startOffset;
    QByteArray _url, _useragent, _buf, _cachedir;
    static QByteArray _proxy;
    bool _cancelled, _successful;
    time_t _lastModified, _serverTime;

    QFile *_file;
    QCryptographicHash _hasher;

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadSuccessful();
    void downloadError(const QString &message);
    /*
     * Note: QThread also provides the signal: void finished();
     * Which can be used to detect when the thread is finished, regardless of success of failure
     */

public slots:

};

#endif // DOWNLOADTHREAD_H
