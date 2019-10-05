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
#include "twoiconsdelegate.h"
#include <QProgressDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QListWidgetItem>
#include <QPushButton>
#include <QCryptographicHash>
#include <QDebug>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/utsname.h>
#include <QScreen>
#include <QSettings>

#include <openssl/pkcs7.h>
#include <openssl/bio.h>
#include <openssl/pem.h>


/* Keep downloaded data global. */
QByteArray AddDialog::_data;
bool AddDialog::_openSSLinitialized = false;

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

    /* Populate sourceforge mirror list. FIXME: this shouldn't be static */
    //ui->mirrorBox->addItem("Astute Internet (Vancouver, BC, Canada)", "astuteinternet");
    ui->mirrorBox->addItem("Ayera Technologies, Inc. (Modesto, CA, USA)", "ayera");
    //ui->mirrorBox->addItem("CFH Cable (USA)", "cfhcable");
    //ui->mirrorBox->addItem("Free France (Paris, France)", "freefr");
    //ui->mirrorBox->addItem("GigeNET (Chicago, IL, USA)", "gigenet");
    ui->mirrorBox->addItem("iWeb Technologies (Montreal, QC, Canada)", "iweb");
    //ui->mirrorBox->addItem("Japan Advanced Institute of Science and Technology (Nomi, Japan)", "jaist");
    ui->mirrorBox->addItem("Liquid Telecom (Kenya)", "liquidtelecom");
    //ui->mirrorBox->addItem("ManagedWay (Detroit, MI, USA)", "managedway");
    //ui->mirrorBox->addItem("National Center for High-Peformance Computing (Taipei, Taiwan)", "nchc");
    //ui->mirrorBox->addItem("NetCologne GmbH (Cologne, Germany)", "netcologne");
    //ui->mirrorBox->addItem("NetIX (Bulgaria)", "netix");
    //ui->mirrorBox->addItem("NewContinuum (West Chicago, IL, USA)", "newcontinuum");
    ui->mirrorBox->addItem("PhoenixNAP (Tempe, AZ, USA)", "phoenixnap");
    //ui->mirrorBox->addItem("Razao Info (Passo Fundo, Brazil)", "razaoinfo");
    ui->mirrorBox->addItem("Silicon Valley Web Hosting (San Jose, CA, USA)", "svwh");
    //ui->mirrorBox->addItem("TENET: The Tertiary Education and Research Network (Wynberg, South Africa)", "tenet");
    ui->mirrorBox->addItem("Centro de Computacao Cientifica e Software Livre (Curitiba, Brazil)", "ufpr");
    ui->mirrorBox->addItem("Versaweb (Las Vegas, NV, USA)", "versaweb");

    /* Check if we have any special proxy server or repo settings */
    if (_i->hasSettings())
    {
        setProxy();
    }

    /* Detect if we are running on a rPi or some ARMv7 device
       Information is used to decide which operating systems to show */
    QFile f("/proc/cpuinfo");
    f.open(f.ReadOnly);
    QByteArray cpuinfo = f.readAll();
    f.close();
    struct utsname uts;

    if (cpuinfo.contains("ARMv7"))
        _device = "armv7";
    else if (cpuinfo.contains("BCM2708"))
        _device = "rpi";
    else if (::uname(&uts) == 0)
        _device = uts.machine;

    f.setFileName("/proc/version");
    /* 'Linux version 3.6.2' -> 36 */
    f.open(f.ReadOnly);
    QByteArray versioninfo = f.readAll();
    f.close();
    _kernelversion = versioninfo.mid(14,4).replace(".", "");

    /* Disable OK button until an image is selected */
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(false);

    if (!QFile::exists("/boot/iscsi.sh"))
    {
        QPushButton *button = new QPushButton(QIcon(":/icons/server.png"), tr("Network settings"), this);
        connect(button, SIGNAL(clicked()), this, SLOT(onProxySettings()));
        ui->buttonBox->addButton(button, QDialogButtonBox::ActionRole);
    }

    if (!_i->networkReady())
    {
        _qpd = new QProgressDialog(tr("Enabling network interface"), QString(), 0,0, this);
        _qpd->show();
        connect(_i, SIGNAL(networkInterfaceUp()), this, SLOT(networkUp()));
        _i->startNetworking();
    }
    else
    {
        if (_data.isEmpty())
            downloadList();
        else
            processIni();
    }
}

AddDialog::~AddDialog()
{
    delete ui;
}

void AddDialog::networkUp()
{
    if (_qpd)
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

    if (_reposerver.startsWith("cifs:") || _reposerver.startsWith("nfs:"))
    {
        generateListFromShare(_reposerver, _repouser, _repopass);
    }
    else
    {
        _downloadCancelled = false;
        _download = new DownloadThread(_reposerver);
        _download->setCacheDirectory(_cachedir);
        connect(_download, SIGNAL(finished()), this, SLOT(downloadComplete()));
        connect(_qpd, SIGNAL(canceled()), this, SLOT(cancelDownload()));

        if (!_reposerver2.isEmpty())
        {
            _download2 = new DownloadThread(_reposerver2);
            _download2->setCacheDirectory(_cachedir);
            connect(_download2, SIGNAL(finished()), this, SLOT(download2Complete()));
            connect(_download, SIGNAL(finished()), _download2, SLOT(start()));
        }

        if (QFile::exists("/mnt/preloaded"))
        {
            connect(_download, SIGNAL(finished()), this, SLOT(generatePreloadedTab()));
        }

        _download->start();
    }
}

void AddDialog::downloadComplete()
{
    if (!_download || _downloadCancelled)
        return; /* Cancelled */

    if (_qpd)
    {
        _qpd->hide();
        _qpd->deleteLater();
        _qpd = NULL;
    }

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

void AddDialog::download2Complete()
{
    if (!_download2 || _downloadCancelled)
        return; /* Cancelled */

    if (_download2->successfull())
    {
        QFile f("/tmp/distro.ini");
        f.open(f.Append);
        f.write(_download2->data());
        f.close();
        processIni();
    }
    else
    {
        /* Do not treat it as fatal if secondary repository is unreachable */
    }

    _download2->deleteLater();
    _download2 = NULL;
}

void AddDialog::cancelDownload()
{
    _downloadCancelled = true;
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
        // _openSSLinitialized = true;
    }
    QByteArray data_uncompressed;
    if (_reposerver.endsWith(".smime"))
        data_uncompressed = _data;
    else
        data_uncompressed = qUncompress(_data);
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
        if (!_cachedir.isEmpty())
            QProcess::execute("rm -rf "+_cachedir);

        _data.clear();
        QFile::remove("/tmp/distro.ini");
        QMessageBox::critical(this, tr("Data corrupt"), tr("Downloaded data corrupt. Signature does not match"), QMessageBox::Close);
        return;
    }

    processIni();
}

void AddDialog::processIni()
{
    _ini = new QSettings("/tmp/distro.ini", QSettings::IniFormat, this);
    QStringList sections = _ini->childGroups();
    QIcon installedIcon(":/icons/hdd.png");
    ui->groupTabs->clear();
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(false);

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

        QString group = _ini->value("group", "Others").toString();
        int groupOrder = _ini->value("grouporder", -1).toInt();
        QString name = _ini->value("name").toString();
        QString description = _ini->value("description").toString();
        QString sizeinmb = QString::number(_ini->value("size", 0).toLongLong()/1024/1024);
        QString localfilename = "/mnt/images/"+name+".img"+_ini->value("memsplit", "").toString();
        localfilename.replace(" ", "_");

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

        /* Search tab corresponding to group */
        QListWidget *osList = NULL;

        for (int i = 0; i < ui->groupTabs->count(); i++)
        {
            if (ui->groupTabs->tabText(i) == group)
            {
                osList = qobject_cast<QListWidget *>(ui->groupTabs->widget(i));
                break;
            }
        }
        if (!osList)
        {
            /* No tab for group yet, create one */
            osList = new QListWidget();
            osList->setIconSize(QSize(128,128));
            osList->setSpacing(2);
            QFont f = osList->font();
            f.setPointSize(16);
            osList->setFont(f);
            osList->setItemDelegate(new TwoIconsDelegate(this));
            connect(osList, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));

            if (groupOrder != -1)
            {
                ui->groupTabs->insertTab(groupOrder, osList, group);
                if (groupOrder == 0)
                {
                    ui->groupTabs->setCurrentIndex(0);
                }
            }
            else
                ui->groupTabs->addTab(osList, group);
        }

        QListWidgetItem *item = new QListWidgetItem(icon, name+" ("+sizeinmb+" MB)\n"+description, osList);
        item->setData(Qt::UserRole, section);

        if (QFile::exists(localfilename))
        {
            item->setData(SecondIconRole, installedIcon);
            item->setData(Qt::BackgroundColorRole, QColor(0xef,0xff,0xef));
        }
    }

    if (sections.contains("berryboot"))
    {
        _ini->beginGroup("berryboot");;
        QString newest_version = _ini->value("version").toString();
        if (newest_version > QString(BERRYBOOT_VERSION))
        {
            QString changelog = _ini->value("description").toString();

            if (QMessageBox::question(this, tr("New BerryBoot version"), changelog+"\n\n"+tr("Would you like to upgrade?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
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

    if (!f.open(f.ReadOnly))
        return "";

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
            QString sha1shared    = sha1file("/boot/shared.tgz");
            QString sha1sharedimg = sha1file("/boot/shared.img");

            qpd.setLabelText(tr("Extracting update to boot partition"));
            QApplication::processEvents();

            if (system("gzip -dc /mnt/tmp/berryupdate.tgz | tar x -C /boot") != 0)
            {
                QMessageBox::critical(this, tr("Error"), tr("Error extracting update"), QMessageBox::Close);
                return;
            }
            QFile::remove(localfile);

            QFileInfo fi("/boot/shared.tgz");
            if (fi.size() && sha1file("/boot/shared.tgz") != sha1shared)
            {
                qpd.setLabelText(tr("Extracting updated shared.tgz"));
                QApplication::processEvents();

                /* Normalize shared after workarounds for Arch/Fedora */
                if (QFile::exists("/mnt/shared/usr/lib/modules"))
                {
                    if (system("mv /mnt/shared/usr/lib /mnt/shared/lib") != 0 || system("mv /mnt/shared/usr/sbin /mnt/shared/sbin") != 0)
                    {
                    }
                    QDir dir;
                    dir.remove("/mnt/shared/usr");
                }

                /* Shared.tgz has changed. Extract it to /mnt */
                if (system("/bin/gzip -dc /boot/shared.tgz | /bin/tar x -C /mnt/shared") != 0)
                {
                    QMessageBox::critical(this, tr("Error"), tr("Error extracting updated shared.tgz"), QMessageBox::Close);
                }
            }
            if (sha1file("/boot/shared.img") != sha1sharedimg)
            {
                qpd.setLabelText(tr("Extracting updated shared.img"));
                QApplication::processEvents();

                /* Normalize shared after workarounds for Arch/Fedora */
                if (QFile::exists("/mnt/shared/usr/lib/modules"))
                {
                    if (system("mv /mnt/shared/usr/lib /mnt/shared/lib") != 0 || system("mv /mnt/shared/usr/sbin /mnt/shared/sbin") != 0)
                    {
                    }
                    QDir dir;
                    dir.remove("/mnt/shared/usr");
                }

                /* Shared.img has changed. Extract it to /mnt */
                if (system("/usr/bin/unsquashfs -d /mnt/shared -f /boot/shared.img") != 0)
                {
                    QMessageBox::critical(this, tr("Error"), tr("Error extracting updated shared.img"), QMessageBox::Close);
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

void AddDialog::onSelectionChanged()
{
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(true);
}

void AddDialog::on_groupTabs_currentChanged(int)
{
    QListWidget *osList = qobject_cast<QListWidget *>(ui->groupTabs->currentWidget());
    bool selected = osList && !osList->selectedItems().isEmpty();
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(selected);
}

void AddDialog::accept()
{
    if (ui->groupTabs->currentIndex() == -1)
        return;
    QListWidget *osList = qobject_cast<QListWidget *>(ui->groupTabs->currentWidget());
    if (!osList->currentItem())
        return;

    QString url, alternateUrl, filename;
    QByteArray description, icon_b64, sha1;
    double size, availablespace = _i->availableDiskSpace();

    if (_ini)
    {
        QString imagesection = osList->currentItem()->data(Qt::UserRole).toString();
        _ini->beginGroup(imagesection);
        url  = _ini->value("url").toString();
        sha1 = _ini->value("sha1").toByteArray();
        filename = _ini->value("name").toString() + ".img" + _ini->value("memsplit", "").toString();
        filename.replace(" ", "_");
        size = _ini->value("size").toDouble() + 10000000; /* Add 10 MB extra for overhead */
        description = _ini->value("description").toByteArray();
        icon_b64    = _ini->value("icon_b64").toByteArray();

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

        /* If sourceforge, take into account local mirror preference */
        QString prefMirror = ui->mirrorBox->itemData(ui->mirrorBox->currentIndex()).toString();
        if (prefMirror != _prefmirror)
        {
            QSettings *s = _i->settings();
            s->beginGroup("repo");
            s->setValue("sourceforgeMirror", prefMirror);
            s->endGroup();
            s->sync();
            _prefmirror = prefMirror;
        }
        if (!prefMirror.isEmpty())
        {
            if (url.contains("downloads.sourceforge.net/"))
                url.replace("downloads.sourceforge.net/", prefMirror+".dl.sourceforge.net/");
            else if (url.contains("sourceforge.net/"))
                url += "?use_mirror="+prefMirror;
        }

        _ini->endGroup();
    }
    else
    {
        /* File on network share */
        filename = osList->currentItem()->data(Qt::UserRole).toString();
        QFileInfo fi(filename);
        size = fi.size() + 10000000; /* Add 10 MB extra for overhead */
        url = "file://"+filename;
        filename = fi.fileName();
    }

    if (size > availablespace)
    {
        QMessageBox::critical(this, tr("Low disk space"), tr("Not enough disk space available to install this OS"), QMessageBox::Close);
        return;
    }

    qDebug() << "Downloading: " << url;
    DownloadDialog dd(url, alternateUrl, filename, DownloadDialog::Image, sha1, this);
    if (!description.isEmpty())
    {
        dd.setAttr("user.description", description);
        dd.setAttr("user.sha1", sha1);
        dd.setAttr("user.icon_b64", icon_b64);
    }
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

    s->beginGroup("repo");
    if (s->contains("url"))
    {
        _reposerver = s->value("url").toByteArray();
        _repouser = s->value("user").toByteArray();
        _repopass = QByteArray::fromBase64(s->value("password").toByteArray());
    }
    else
    {
        _reposerver = DEFAULT_REPO_SERVER;
        _repouser = _repopass = "";
    }
    _reposerver2 = s->value("url2").toByteArray();

    _prefmirror = s->value("sourceforgeMirror").toString();
    if (!_prefmirror.isEmpty())
    {
        int idx = ui->mirrorBox->findData(_prefmirror);
        if (idx != -1)
        {
            ui->mirrorBox->setCurrentIndex(idx);
        }
    }

    s->endGroup();
}

void AddDialog::generateListFromShare(const QByteArray &url, QByteArray username, QByteArray password)
{
    QApplication::processEvents();
    QByteArray shareType, share;

    if (url.startsWith("cifs:"))
    {
        shareType = "cifs";
        share = url.mid(5);
        if (username.isEmpty())
            username = "guest";
    }
    else if (url.startsWith("nfs:"))
    {
        shareType = "nfs";
        share = url.mid(4);
    }
    else
    {
        return;
    }

    _i->loadFilesystemModule(shareType);

    QDir dir("/share");
    if (dir.exists())
    {
        QProcess::execute("umount /share");
    }
    else
    {
        dir.mkdir("/share");
    }

    QStringList args;
    args << "-t" << shareType << share << "/share";

    if (!username.isEmpty())
    {
        args << "-o" << "username="+username;
        if (!password.isEmpty())
            args << "-o" << "password="+password;
    }
    if (QProcess::execute("mount", args) != 0)
    {
        dir.rmdir("/share");
        QMessageBox::critical(this, tr("Mount error"), tr("Error mounting network share %1").arg(QString(share)), QMessageBox::Ok);
    }
    else
    {
        _ini = NULL;
        ui->groupTabs->clear();

        /* Create tab */
        QListWidget *osList = new QListWidget();
        osList->setIconSize(QSize(128,128));
        osList->setSpacing(2);
        QFont f = osList->font();
        f.setPointSize(16);
        osList->setFont(f);
        connect(osList, SIGNAL(itemSelectionChanged()), this, SLOT(onSelectionChanged()));
        ui->groupTabs->addTab(osList, tr("Network share"));

        QStringList namefilters;
        namefilters << "*.img*";

        QFileInfoList list = dir.entryInfoList(namefilters, QDir::Files, QDir::Name);
        foreach (QFileInfo fi, list)
        {
            QString name = fi.fileName().replace('_',' ');
            QString sizeinmb = QString::number(fi.size()/1024/1024);
            QListWidgetItem *item = new QListWidgetItem(name+" ("+sizeinmb+" MB)", osList);
            item->setData(Qt::UserRole, fi.absoluteFilePath() );
        }
    }

    if (_qpd)
    {
        _qpd->hide();
        _qpd->deleteLater();
        _qpd = NULL;
    }
}

QByteArray AddDialog::getXattr(const QByteArray &filename, const QByteArray &key)
{
    char buf[32*1024];
    int len;
    QByteArray result;

    len = ::getxattr(filename.constData(), key.constData(), buf, sizeof(buf));
    if (len > 0)
        result = QByteArray(buf, len);

    return result;
}

void AddDialog::generatePreloadedTab()
{
    QByteArray ini;
    int nr = 1;

    QDir dir("/mnt/preloaded");
    QStringList namefilters;
    namefilters << "*.img*";

    QFileInfoList list = dir.entryInfoList(namefilters, QDir::Files, QDir::Name);
    foreach (QFileInfo fi, list)
    {
        QByteArray name = fi.fileName().replace('_',' ').toLatin1();
        QByteArray memsplit;
        QByteArray absfilename = fi.absoluteFilePath().toLatin1();
        QByteArray description = getXattr(absfilename, "user.description");
        QByteArray icon_b64    = getXattr(absfilename, "user.icon_b64");
        QByteArray sha1        = getXattr(absfilename, "user.sha1");

        int imgpos = name.lastIndexOf(".img");
        if (imgpos != -1 && imgpos+5 < name.size())
        {
            memsplit = name.mid(imgpos+4);
        }
        name = name.left(imgpos);

        ini += "[preloaded"+QByteArray::number(nr++)+"]\n";
        ini += "name="+name+"\n";
        if (!memsplit.isEmpty())
            ini += "memsplit="+memsplit+"\n";
        ini += "description="+description.replace("\n", "\\n")+"\n";
        ini += "group=Preloaded\n";
        ini += "size="+QByteArray::number(fi.size())+"\n";
        if (!sha1.isEmpty())
            ini += "sha1="+sha1+"\n";
        ini += "url=file://"+absfilename+"\n";
        if (!icon_b64.isEmpty())
            ini += "icon_b64="+icon_b64.replace("\n", "\\n")+"\n";
        ini += "\n";
    }
    qDebug() << ini;

    QFile f("/tmp/distro.ini");
    f.open(f.Append);
    f.write(ini);
    f.close();
    processIni();
}
