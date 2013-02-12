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

#include "downloadthread.h"
#include <QFile>
#include <QDebug>
#include <curl/curl.h>

QByteArray DownloadThread::_proxy;


DownloadThread::DownloadThread(const QByteArray &url, const QString &localfilename, QObject *parent) :
    QThread(parent), _lastDlTotal(0), _lastDlNow(0), _url(url), _cancelled(false), _successful(false),
    _lastModified(0), _serverTime(0), _file(NULL), _hasher(QCryptographicHash::Sha1)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
    if (!localfilename.isEmpty())
        _file = new QFile(localfilename, this);
}

DownloadThread::~DownloadThread()
{
    wait();
    curl_global_cleanup();
}

void DownloadThread::setProxy(const QByteArray &proxy)
{
    _proxy = proxy;
}

QByteArray DownloadThread::proxy()
{
    return _proxy;
}

void DownloadThread::setUserAgent(const QByteArray &ua)
{
    _useragent = ua;
}

/* Curl write callback function, let it call the object oriented version */
static size_t _curl_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    return static_cast<DownloadThread *>(userdata)->_writeData(ptr, size * nmemb);
}

static int _curl_progress_callback(void *userdata, double dltotal, double dlnow, double ultotal, double ulnow)
{
    return (static_cast<DownloadThread *>(userdata)->_progress(dltotal, dlnow, ultotal, ulnow) == false);
}

static size_t _curl_header_callback( void *ptr, size_t size, size_t nmemb, void *userdata)
{
    int len = size*nmemb;
    QByteArray headerstr((char *) ptr, len);
    static_cast<DownloadThread *>(userdata)->_header(headerstr);

    return len;
}


void DownloadThread::run()
{
    if (_file && !_file->open(QFile::WriteOnly))
    {
        emit downloadError(tr("Error opening output file"));
        return;
    }

    char errorBuf[CURL_ERROR_SIZE] = {0};
    _c = curl_easy_init();
    curl_easy_setopt(_c, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(_c, CURLOPT_WRITEFUNCTION, &_curl_write_callback);
    curl_easy_setopt(_c, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(_c, CURLOPT_PROGRESSFUNCTION, &_curl_progress_callback);
    curl_easy_setopt(_c, CURLOPT_PROGRESSDATA, this);
    curl_easy_setopt(_c, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(_c, CURLOPT_URL, _url.constData());
    curl_easy_setopt(_c, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(_c, CURLOPT_MAXREDIRS, 10);
    curl_easy_setopt(_c, CURLOPT_ERRORBUFFER, errorBuf);
    curl_easy_setopt(_c, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(_c, CURLOPT_HEADERFUNCTION, &_curl_header_callback);
    curl_easy_setopt(_c, CURLOPT_HEADERDATA, this);

    if (!_useragent.isEmpty())
        curl_easy_setopt(_c, CURLOPT_USERAGENT, _useragent.constData());
    if (!_proxy.isEmpty())
        curl_easy_setopt(_c, CURLOPT_PROXY, _proxy.constData());

    CURLcode ret = curl_easy_perform(_c);

    if (_file)
        _file->close();
    curl_easy_cleanup(_c);

    qDebug() << "curl ret" << ret;

    switch (ret)
    {
        case CURLE_OK:
            _successful = true;
            emit downloadSuccessful();
            break;
        case CURLE_WRITE_ERROR:
            deleteDownloadedFile();
            emit downloadError(tr("Error writing to SD card"));
        case CURLE_ABORTED_BY_CALLBACK:
            qDebug() << "Download cancelled";
            break;
        default:
            deleteDownloadedFile();
            emit downloadError(errorBuf);
    }
}

size_t DownloadThread::_writeData(const char *buf, size_t len)
{
    _hasher.addData(buf, len);
    if (_file)
    {
        return _file->write(buf, len);
    }
    else
    {
        _buf.append(buf, len);
        return len;
    }
}

bool DownloadThread::_progress(double dltotal, double dlnow, double /*ultotal*/, double /*ulnow*/)
{
    /* libcurl regulary calls back even if no progress is made, but we only signal if there is progress */
    if (dltotal != _lastDlTotal || dlnow != _lastDlNow)
    {
        _lastDlTotal = dltotal;
        _lastDlNow   = dlnow;

        emit downloadProgress(dlnow, dltotal);
    }

    return !_cancelled;
}


void DownloadThread::_header(QByteArray &header)
{
    qDebug() << "Received HTTP header:" << header;

    if (header.startsWith("Date: "))
    {
        _serverTime = curl_getdate(header.data()+6, NULL);
    }
    else if (header.startsWith("Last-Modified: "))
    {
        _lastModified = curl_getdate(header.data()+15, NULL);
    }
}

void DownloadThread::cancelDownload()
{
    _cancelled = true;
    deleteDownloadedFile();
}

QByteArray DownloadThread::data()
{
    return _buf;
}

bool DownloadThread::successfull()
{
    return _successful;
}

QByteArray DownloadThread::sha1()
{
    return _hasher.result().toHex();
}

time_t DownloadThread::lastModified()
{
    return _lastModified;
}

time_t DownloadThread::serverTime()
{
    return _serverTime;
}

void DownloadThread::deleteDownloadedFile()
{
    if (_file)
        _file->remove();
}
