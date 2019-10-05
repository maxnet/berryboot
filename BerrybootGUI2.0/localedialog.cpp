/* Berryboot -- welcome dialog
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


#include "localedialog.h"
#include "ui_localedialog.h"
#include "installer.h"
#include "greenborderdialog.h"
#include "wifidialog.h"
#include "downloadthread.h"
#include "wificountrydetector.h"

#include <unistd.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <QStringList>
#include <QFile>
#include <QProgressDialog>
#include <QRegExp>
#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QMessageBox>

#define GEOIP_SERVER "http://geoip.ubuntu.com/lookup"


LocaleDialog::LocaleDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LocaleDialog),
    _i(i)
{
    ui->setupUi(this);

    /* Populate timezone list */
    QStringList timezones;
    QFile f(":/timezones.txt");
    f.open(f.ReadOnly);
    while ( f.bytesAvailable() )
        timezones.append( f.readLine().trimmed() );
    f.close();

    ui->timezoneCombo->addItems(timezones);

    /* Populate wifi country information */
    QStringList wificountries;
    f.setFileName(":/countries.txt");
    f.open(f.ReadOnly);
    while ( f.bytesAvailable() )
        wificountries.append( f.readLine().trimmed() );
    f.close();

    ui->wificountryCombo->addItems(wificountries);

    /* Populate keyboard information */
    QDir dir(":/qmap");
    QStringList list = dir.entryList(QDir::Files);

    if (!list.empty())
    {
        ui->keybCombo->clear();
        ui->keybCombo->addItems(list);
        ui->keybCombo->setCurrentIndex(ui->keybCombo->findText("us"));
    }

    ui->locationLabel->setText(tr("-no network connection-"));
    checkIfNetworkIsUp();

    /* Paint green border */
    _gbd = new GreenBorderDialog();
    _gbd->showMaximized();

    //ui->overscanGroupBox->setHidden(!_i->hasOverscanSettings());
    ui->fixMACbox->setHidden(!_i->hasDynamicMAC());
    ui->keybtestEdit->setFocus();

    if (QFile::exists("/boot/wpa_supplicant.conf"))
    {
        ui->wifiRadio->setText(tr("Wifi (using existing wpa_supplicant.conf settings)"));
        ui->wifiRadio->setChecked(true);
    }

    //QTimer::singleShot(3000, this, SLOT(checkIfNetworkNeedsDrivers()));
    QTimer::singleShot(100, _i, SLOT(loadDrivers()));
}

LocaleDialog::~LocaleDialog()
{
    delete ui;
    delete _gbd;
}

void LocaleDialog::checkIfNetworkIsUp()
{
    /* Determinate location */
    if (_i->networkReady())
    {
        DownloadThread *download = new DownloadThread(GEOIP_SERVER);
        connect(download, SIGNAL(finished()), this, SLOT(downloadComplete()));
        download->start();
        if (_i->eth0Up() && _i->cpuinfo().contains("BCM2835"))
            checkFlow();
    }
    else
    {
        /* Network not up yet, check again in a tenth of a second */
        QTimer::singleShot(100, this, SLOT(checkIfNetworkIsUp()));
    }
}

void LocaleDialog::checkIfNetworkNeedsDrivers()
{
    QFile f("/sys/class/net/eth0");
    if (!f.exists())
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QApplication::processEvents();
        _i->loadDrivers();
        QApplication::restoreOverrideCursor();
    }
}

void LocaleDialog::checkFlow()
{
    struct ethtool_cmd c;
    struct ifreq ifr;
    ::memset(&c, 0, sizeof(c));
    ::memset(&ifr, 0, sizeof(ifr));
    ::strcpy(ifr.ifr_name, "eth0");
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0)
        return;

    c.cmd = ETHTOOL_GSET;
    ifr.ifr_data = (char *) &c;
    if (::ioctl(fd, SIOCETHTOOL, &ifr) == 0)
    {
        if ( c.speed == SPEED_1000
             && !(c.advertising & c.lp_advertising & ADVERTISED_Pause)
             && !(c.advertising & c.lp_advertising & ADVERTISED_Asym_Pause))
        {
            QMessageBox::critical(this, tr("Check your switch settings"),
                                  tr("Flow control seems to be disabled on your Ethernet switch.\n"
                                     "This can cause network performance problems on the Pi 3+\n"
                                     "If possible change your switch settings to enable flow control."), QMessageBox::Close);
        }
    }

    ::close(fd);
}

void LocaleDialog::downloadComplete()
{
    DownloadThread *reply = qobject_cast<DownloadThread *>( sender() );

    /* Replies are in format:

     <Response>
        <Ip>1.2.3.4</Ip>
        <Status>OK</Status>
        <CountryCode>NL</CountryCode>
        <CountryCode3>NLD</CountryCode3>
        <CountryName>Netherlands</CountryName>
        <RegionCode>15</RegionCode>
        <RegionName>Overijssel</RegionName>
        <City>Enschede</City>
        <ZipPostalCode/>
        <Latitude>52.2195</Latitude>
        <Longitude>6.8912</Longitude>
        <AreaCode>0</AreaCode>
        <TimeZone>Europe/Amsterdam</TimeZone>
      </Response>
    */

    if (reply->successfull())
    {
        QByteArray data = reply->data();
        //qDebug() << "Data received" << data;

        QRegExp regexp("\\<CountryCode\\>(.*)\\<\\/CountryCode\\>.*\\<CountryName\\>(.*)\\<\\/CountryName\\>.*\\<TimeZone\\>(.*)\\<\\/TimeZone\\>");
        if ( regexp.indexIn(data) != -1 )
        {
            QString cc = regexp.cap(1).toLower();
            QString countryname = regexp.cap(2);
            QString tz = regexp.cap(3);

            ui->locationLabel->setText(countryname);
            int idx    = ui->timezoneCombo->findText(tz);
            if (idx != -1)
                ui->timezoneCombo->setCurrentIndex(idx);
            else
                ui->timezoneCombo->setEditText(tz);

            // Check if the country is known to use a specific keyboard layout
            QSettings country2keyboard(":/country2keyboard.ini", QSettings::IniFormat);
            if (country2keyboard.contains(cc))
            {
                idx = ui->keybCombo->findText(country2keyboard.value(cc).toString());
                if (idx != -1)
                    ui->keybCombo->setCurrentIndex(idx);
            }
        }
    }

    reply->deleteLater();
}

void LocaleDialog::accept()
{
    _i->setKeyboardLayout(ui->keybCombo->currentText());
    _i->setTimezone(ui->timezoneCombo->currentText());
    _i->setDisableOverscan(ui->disableOverscanRadio->isChecked());
    _i->setFixateMAC(_i->hasDynamicMAC() && ui->fixMACbox->isChecked());
    bool wifi = ui->wifiRadio->isChecked();
    QByteArray wifiCountry = ui->wificountryCombo->currentText().toLatin1();

    if (wifi && wifiCountry == "--" && !QFile::exists("/boot/wpa_supplicant.conf"))
    {
        QMessageBox::critical(this, tr("No wifi country selected"), tr("Please select a wifi country from the list"), QMessageBox::Ok);
        return;
    }

    if (!wifi && !_i->networkReady())
    {
        if (QMessageBox::question(this, tr("Check network"),
                                  tr("There does not seem to be network connectivity. Are you sure you want to use a wired connection?"),
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        {
            return;
        }
    }

    if (_gbd)
        _gbd->hide();

    QDialog::accept();
    if (wifi && !QFile::exists("/boot/wpa_supplicant.conf"))
    {
        QProcess::execute("/usr/sbin/iw reg set "+wifiCountry);
        WifiDialog wd(_i);
        wd.setCountry(wifiCountry);
        wd.exec();
    }
    else if (!wifi && QFile::exists("/boot/wpa_supplicant.conf"))
    {
        QFile::remove("/boot/wpa_supplicant.conf");
    }
}

void LocaleDialog::reject()
{
    if (QMessageBox::question(this, tr("Skip configuration?"),
                              tr("Are you sure you want to skip configuration now? If you say yes, you will be prompted again on next boot."),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        _i->setSkipConfig(true);
        _i->setDisableOverscan(true);
        _i->setFixateMAC(false);
        if (_gbd)
            _gbd->hide();
        QDialog::reject();
    }
}

void LocaleDialog::on_keybCombo_currentIndexChanged(const QString &layout)
{
    _i->setKeyboardLayout(layout);
}

void LocaleDialog::on_wifiRadio_clicked()
{
    ui->wificountryLabel->setEnabled(true);
    ui->wificountryCombo->setEnabled(true);

    if (!QFile::exists("/boot/wpa_supplicant.conf"))
    {
        WifiCountryDetector *w = new WifiCountryDetector(_i, this);
        QProgressDialog *qpd = new QProgressDialog(QString(), QString(), 0, 0, this);
        connect(w, SIGNAL(finished()), qpd, SLOT(deleteLater()));
        connect(w, SIGNAL(statusUpdate(QString)), qpd, SLOT(setLabelText(QString)));
        connect(w, SIGNAL(countryDetected(QByteArray,int)), this, SLOT(wifiCountryDetected(QByteArray,int)));
        qpd->show();
        w->start();
    }
}


void LocaleDialog::on_ethRadio_clicked()
{
    ui->wificountryLabel->setEnabled(false);
    ui->wificountryCombo->setEnabled(false);
}

void LocaleDialog::wifiCountryDetected(QByteArray cc, int numAPs)
{
    ui->locationLabel->setText(QString("%1 (advertised by %2 APs)").arg(cc, QString::number(numAPs)));
    int idx = ui->wificountryCombo->findText(cc, Qt::MatchExactly);
    if (idx != -1)
    {
        ui->wificountryCombo->setCurrentIndex(idx);
    }

    if (ui->timezoneCombo->currentIndex() == 0 && ui->keybCombo->currentText() == "us")
    {
        // Look up timezone for country (there could be more than one though)
        QSettings country2timezone(":/country2timezone.ini", QSettings::IniFormat);
        if (country2timezone.contains(cc))
        {
            idx = ui->timezoneCombo->findText(country2timezone.value(cc).toString());
            if (idx != -1)
                ui->timezoneCombo->setCurrentIndex(idx);
        }

        // Check if the country is known to use a specific keyboard layout
        QSettings country2keyboard(":/country2keyboard.ini", QSettings::IniFormat);
        if (country2keyboard.contains(cc))
        {
            idx = ui->keybCombo->findText(country2keyboard.value(cc).toString());
            if (idx != -1)
                ui->keybCombo->setCurrentIndex(idx);
        }
    }

    ui->keybtestEdit->setFocus();
}
