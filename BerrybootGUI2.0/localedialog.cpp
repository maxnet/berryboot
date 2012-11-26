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

#include <QStringList>
#include <QFile>
#include <QProgressDialog>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QRegExp>
#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QTimer>

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

    //ui->keybtestEdit->setFocus();
    ui->keybtestEdit->setHidden(true);
    ui->label_7->setHidden(true);
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
        QNetworkAccessManager *netaccess = new QNetworkAccessManager(this);
        QNetworkReply *reply = netaccess->get(QNetworkRequest(QUrl(GEOIP_SERVER)));
        connect(reply, SIGNAL(finished()), this, SLOT(downloadComplete()));
    }
    else
    {
        /* Network not up yet, check again in a tenth of a second */
        QTimer::singleShot(100, this, SLOT(checkIfNetworkIsUp()));
    }
}

void LocaleDialog::downloadComplete()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );

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

    if (reply->error() == reply->NoError)
    {
        QByteArray data = reply->readAll();
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

void LocaleDialog::done(int r)
{
    _i->setKeyboardLayout(ui->keybCombo->currentText());
    _i->setTimezone(ui->timezoneCombo->currentText());
    _i->setDisableOverscan(ui->disableOverscanRadio->isChecked());
    bool wifi = ui->wifiRadio->isChecked();

    if (_gbd)
        _gbd->hide();

    QDialog::done(r);
    if (wifi)
    {
        WifiDialog wd(_i);
        wd.exec();
    }
}

void LocaleDialog::on_keybCombo_currentIndexChanged(const QString &layout)
{
    _i->setKeyboardLayout(layout);
}
