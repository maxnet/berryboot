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

#include "adddialog.h"
#include "ui_adddialog.h"
#include "installer.h"
#include "downloaddialog.h"
#include "downloadthread.h"
#include "networksettingsdialog.h"
#include <QProgressDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QtGui/QListWidgetItem>
#include <QtGui/QPushButton>
#include <QCryptographicHash>
#include <QDebug>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <QScreen>
#include <QSettings>

#include <openssl/pkcs7.h>
#include <openssl/bio.h>
#include <openssl/pem.h>


/* Keep downloaded data global. */
QByteArray AddDialog::_data;
bool AddDialog::_openSSLinitialized = false;

/* parsedate.c */
extern "C" time_t parse_date(char *p, time_t *now);

#define DEFAULT_REPO_SERVER   "http://dl.berryboot.com/distro.zsmime"


AddDialog::AddDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddDialog),
    _qpd(NULL), _i(i), _cachedir("/mnt/tmp/cache"), _ini(NULL), _reposerver(DEFAULT_REPO_SERVER), _download(NULL)
{
    ui->setupUi(this);

#ifdef Q_WS_QWS
    /* Make dialog smaller if using a small screen (e.g. SDTV) */
    QScreen *scr = QScreen::instance();
    if (scr->height() < 600)
        resize(width(), 400);
#endif

    /* Check if we have any special proxy server or repo settings */
    if (_i->hasSettings())
    {
        setProxy();
        QSettings *s = _i->settings();
        s->beginGroup("repo");
        if (s->contains("url"))
            _reposerver = s->value("url").toByteArray();
        s->endGroup();
    }

    /* Detect if we are running on a rPi or some ARMv7 device
       Information is used to decide which operating systems to show */
    QFile f("/proc/cpuinfo");
    f.open(f.ReadOnly);
    QByteArray cpuinfo = f.readAll();
    f.close();

    if (cpuinfo.contains("ARMv7"))
        _device = "armv7";
    else if (cpuinfo.contains("BCM2708"))
        _device = "rpi";
    else
        _device = "other";

    f.setFileName("/proc/version");
    /* 'Linux version 3.6.2' -> 36 */
    f.open(f.ReadOnly);
    QByteArray versioninfo = f.readAll();
    f.close();
    _kernelversion = versioninfo.mid(14,4).replace(".", "");

    /* Disable OK button until an image is selected */
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(false);
    QPushButton *button = new QPushButton(QIcon(":/icons/server.png"), tr("Network settings"), this);
    connect(button, SIGNAL(clicked()), this, SLOT(onProxySettings()));
    ui->buttonBox->addButton(button, QDialogButtonBox::ActionRole);
    connect(_i, SIGNAL(networkInterfaceUp()), this, SLOT(networkUp()));

    if (!_i->networkReady())
    {
        _qpd = new QProgressDialog(tr("Enabling network interface"), QString(), 0,0, this);
        _qpd->show();
        _i->startNetworking();
    }
    else
    {
        if (_data.isEmpty())
            downloadList();
        else
            processData();
    }
}

AddDialog::~AddDialog()
{
    QFile::remove("/tmp/distro.ini");
    delete ui;
}

void AddDialog::networkUp()
{
    _qpd->hide();

 /*   if (!_i->networkReady())
    {
        QMessageBox::critical(this, tr("No network"), tr("No network connection available (or no DHCP server)"), QMessageBox::Close);
        return;
    } */

    if (_data.isEmpty())
        downloadList();
}

void AddDialog::downloadList()
{
    _data.clear();

    if (_qpd)
        _qpd->deleteLater();
    _qpd = new QProgressDialog(tr("Downloading list of available distributions"), tr("Cancel"), 0,0, this);
    _qpd->show();

    /*if (!QFile::exists(_cachedir))
    {
        QDir dir;
        dir.mkdir(_cachedir);
    }*/
    _download = new DownloadThread(_reposerver);
    connect(_download, SIGNAL(finished()), this, SLOT(downloadComplete()));
    connect(_qpd, SIGNAL(canceled()), this, SLOT(cancelDownload()));
    _download->start();
}

void AddDialog::downloadComplete()
{
    if (!_download)
        return; /* Cancelled */

    _qpd->hide();
    _qpd->deleteLater();
    _qpd = NULL;

    if (!_download->successfull())
    {
        QMessageBox::critical(this, tr("Download error"), tr("Error downloading distribution list from Internet"), QMessageBox::Ok);
    }
    else
    {
        _data = _download->data();

        time_t localTime  = time(NULL);
        time_t serverTime = _download->serverTime();
        qDebug() << "Date from server: " << serverTime << "local time:" << localTime;

        if (serverTime > localTime)
        {
            qDebug() << "Setting time to server time";
            struct timeval tv;
            tv.tv_sec = serverTime;
            tv.tv_usec = 0;
            settimeofday(&tv, NULL);
        }

        processData();
    }

    _download->deleteLater();
    _download = NULL;
}

void AddDialog::cancelDownload()
{
    if (_qpd)
    {
        _qpd->deleteLater();
        _qpd = NULL;
    }
    if (_download)
    {
        _download->cancelDownload();
        _download->deleteLater();
        _download = NULL;
    }
}

bool AddDialog::verifyData()
{
    bool verified = false;

    if (!_openSSLinitialized)
    {
        OpenSSL_add_all_algorithms();
        _openSSLinitialized = true;
    }
    QByteArray data_uncompressed = qUncompress(_data);
    QString certfilename;

    if (QFile::exists("/boot/berryboot.crt"))
        certfilename = "/boot/berryboot.crt";
    else
        certfilename = ":/berryboot.crt";

    QFile f(certfilename);
    f.open(f.ReadOnly);
    QByteArray cert = f.readAll();
    f.close();
    X509 *scert = NULL;
    STACK_OF(X509) *scerts = sk_X509_new_null();
    PKCS7 *p7 = NULL;
    BIO *certbio = BIO_new_mem_buf(cert.data(), cert.size());
    BIO *inbio = BIO_new_mem_buf(data_uncompressed.data(), data_uncompressed.size());
    BIO *cont = NULL, *outbio = NULL;
    X509_STORE *st = X509_STORE_new();
    int flags = PKCS7_NOINTERN | PKCS7_NOVERIFY;
    scert = PEM_read_bio_X509(certbio, NULL, 0, NULL);

    sk_X509_push(scerts, scert);
    p7 = SMIME_read_PKCS7(inbio, &cont);

    if (p7)
    {
        outbio = BIO_new_file("/tmp/distro.ini", "w");

        if (PKCS7_verify(p7, scerts, st, cont, outbio, flags))
            verified = true;
    }

    if (p7)
        PKCS7_free(p7);
    if (scerts)
        sk_X509_pop_free(scerts, X509_free);
    if (inbio)
        BIO_free(inbio);
    if (outbio)
        BIO_free(outbio);
    if (cont)
        BIO_free(cont);
    if (st)
        X509_STORE_free(st);

    return verified;
}



void AddDialog::processData()
{
    if (!verifyData())
    {
/*        if (_cache)
            _cache->clear();*/
        QMessageBox::critical(this, tr("Data corrupt"), tr("Downloaded data corrupt. Signature does not match"), QMessageBox::Close);
        return;
    }

    _ini = new QSettings("/tmp/distro.ini", QSettings::IniFormat, this);
    QStringList sections = _ini->childGroups();
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(false);
    ui->osList->clear();

    foreach (QString section, sections)
    {
        if (section == "berryboot")
            continue;

        _ini->beginGroup(section);
        if (_ini->contains("device") && _ini->value("device").toString() != _device && _ini->value("device").toString() != _device+_kernelversion)
        {
            _ini->endGroup();
            continue;
        }

        QString name = _ini->value("name").toString();
        QString description = _ini->value("description").toString();
        QString sizeinmb = QString::number(_ini->value("size", 0).toLongLong()/1024/1024);
        QIcon   icon;
        if (_ini->contains("icon_b64"))
        {
            _ini->setValue("icon", QByteArray::fromBase64(_ini->value("icon_b64").toByteArray() ));
        }
        if (_ini->contains("icon"))
        {
            QPixmap pix;
            pix.loadFromData(_ini->value("icon").toByteArray());
            icon = pix;
        }
        _ini->endGroup();

        QListWidgetItem *item = new QListWidgetItem(icon, name+" ("+sizeinmb+" MB)\n"+description, ui->osList);
        item->setData(Qt::UserRole, section);
    }

    if (sections.contains("berryboot"))
    {
        _ini->beginGroup("berryboot");;
        QString newest_version = _ini->value("version").toString();
        if (newest_version != BERRYBOOT_VERSION)
        {
            QString changelog = _ini->value("description").toString();

            if (QMessageBox::question(this, tr("New BerryBoot version available"), changelog+"\n\n"+tr("Would you like to upgrade?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
            {
                if (_i->availableDiskSpace() < 50000000)
                    QMessageBox::critical(this, tr("Low disk space"), tr("Less than 50 MB available. Refusing to update"), QMessageBox::Close);
                else
                    selfUpdate(_ini->value("url").toString(), _ini->value("sha1").toString() );
            }
        }
        _ini->endGroup();
    }

}

QByteArray AddDialog::sha1file(const QString &filename)
{
    QFile f(filename);
    QByteArray buf;
    QCryptographicHash h(QCryptographicHash::Sha1);

    f.open(f.ReadOnly);
    while ( f.bytesAvailable() )
    {
        buf = f.read(4096);
        h.addData(buf);
    }

    f.close();
    return h.result().toHex();
}

void AddDialog::selfUpdate(const QString &updateurl, const QString &sha1)
{
    setEnabled(false);
    QString localfile = "/mnt/tmp/berryupdate.tgz";

    DownloadDialog dd(updateurl, "", "berryupdate.tgz", DownloadDialog::Update, sha1, this);

    if ( dd.exec() == dd.Accepted)
    {
        QProgressDialog qpd(tr("Recalculating SHA1"), QString(),0,0, this);
        qpd.show();

        /* Recalculate SHA1 one more time to rule out SD card corruption */
        if (sha1file(localfile) != sha1)
        {
            QMessageBox::critical(this, tr("Error"), tr("Data on SD card corrupt"), QMessageBox::Close);
            QFile::remove(localfile);
            return;
        }
        else
        {
            qpd.setLabelText(tr("Mounting system partition"));
            QApplication::processEvents();

            _i->mountSystemPartition();
            QString sha1shared = sha1file("/boot/shared.tgz");

            qpd.setLabelText(tr("Extracting update to boot partition"));
            QApplication::processEvents();

            if (system("gzip -dc /mnt/tmp/berryupdate.tgz | tar x -C /boot") != 0)
            {
                QMessageBox::critical(this, tr("Error"), tr("Error extracting update"), QMessageBox::Close);
                return;
            }
            QFile::remove(localfile);

            if (sha1file("/boot/shared.tgz") != sha1shared)
            {
                qpd.setLabelText(tr("Extracting updated shared.tgz"));
                QApplication::processEvents();

                /* Shared.tgz has changed. Extract it to /mnt */
                if (system("/bin/gzip -dc /boot/shared.tgz | /bin/tar x -C /mnt/shared") != 0)
                {
                    QMessageBox::critical(this, tr("Error"), tr("Error extracting updated shared.tgz"), QMessageBox::Close);
                }
            }

            if (QFile::exists("/boot/post-update.sh"))
            {
                qpd.setLabelText(tr("Running post-update script"));
                QProcess::execute("/bin/sh /boot/post-update.sh");
                QFile::remove("/boot/post-update.sh");
            }

            qpd.setLabelText(tr("Unmounting boot partition"));
            QApplication::processEvents();

            _i->umountSystemPartition();

            qpd.setLabelText(tr("Finish writing to disk (sync)"));
            QApplication::processEvents();

            sync();
            qpd.hide();
            QMessageBox::information(this, tr("Update complete"), tr("Press 'close' to reboot"), QMessageBox::Close);
            _i->reboot();
        }
    }

    setEnabled(true);
}

void AddDialog::on_osList_currentRowChanged(int)
{
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(true);
}

void AddDialog::accept()
{
    QString imagesection = ui->osList->currentItem()->data(Qt::UserRole).toString();
    _ini->beginGroup(imagesection);
    QString url  = _ini->value("url").toString();
    QString alternateUrl;
    QString sha1 = _ini->value("sha1").toString();
    QString filename = _ini->value("name").toString() + ".img" + _ini->value("memsplit", "").toString();
    filename.replace(" ", "_");
    double size = _ini->value("size").toDouble() + 10000000; /* Add 10 MB extra for overhead */
    double availablespace = _i->availableDiskSpace();

    /* If mirrors are available, select a random one */
    QStringList mirrors;
    QStringList keys = _ini->childKeys();
    foreach (QString key, keys)
    {
        if (key.startsWith("mirror"))
        {
            mirrors.append(_ini->value(key).toString());
        }
    }
    if (!mirrors.isEmpty())
    {
        /* Try a random mirror first, and the main site if downloading from mirror fails */
        alternateUrl = url;
        qsrand(QTime::currentTime().msec());
        url = mirrors.at(qrand() % mirrors.count());
    }

    _ini->endGroup();

    if (size > availablespace)
    {
        QMessageBox::critical(this, tr("Low disk space"), tr("Not enough disk space available to install this OS"), QMessageBox::Close);
        return;
    }

    DownloadDialog dd(url, alternateUrl, filename, DownloadDialog::Image, sha1, this);
    hide();
    dd.exec();

    QDialog::accept();
}

void AddDialog::onProxySettings()
{
    NetworkSettingsDialog ns(_i, this);

    if (ns.exec() == ns.Accepted)
    {
        setProxy();
        downloadList();
    }
}

void AddDialog::setProxy()
{
    QSettings *s = _i->settings();
    s->beginGroup("proxy");
    if (s->contains("type"))
    {
        QByteArray proxy, user, pass;
        proxy = s->value("hostname").toByteArray()+":"+s->value("port").toByteArray();
        user = s->value("user").toByteArray();
        pass = QByteArray::fromBase64(s->value("password").toByteArray());
        if (!user.isEmpty() && !pass.isEmpty())
            proxy = user+":"+pass+"@"+proxy;

        DownloadThread::setProxy(proxy);
    }
    else
    {
        DownloadThread::setProxy("");
    }
    s->endGroup();
}
