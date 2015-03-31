/* Berryboot -- Wifi settings dialog
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

/* Qt has a bearer plugin that could be used to detect available networks,
 * but it does not work without additional dependencies. So we simply call wpa_supplicant / wpa_cli instead
 * A better alternative might be to use the low-level SIOCSIWSCAN and related ioctls, but this was faster to implement
 */

#include "wifidialog.h"
#include "ui_wifidialog.h"
#include "installer.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QProgressDialog>
#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QListWidgetItem>
#include <QTime>
#include <QDebug>

WifiDialog::WifiDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WifiDialog),
    _i(i),
    _proc(NULL)
{
    ui->setupUi(this);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(pollScanResults()));

    /* Disable OK button until a network is selected */
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(false);

    QProgressDialog qpd(tr("Loading drivers"), QString(), 0, 0, this);
    qpd.show();
    QApplication::processEvents();
    i->loadDrivers();

    /* Wait up to 2 seconds for wifi device to appear */
    QTime t;
    t.start();
    while (t.elapsed() < 2000 && !QFile::exists("/sys/class/net/wlan0") )
    {
        QApplication::processEvents(QEventLoop::WaitForMoreEvents, 250);
    }

    if ( QFile::exists("/sys/class/net/wlan0") )
    {
        qpd.setLabelText(tr("Starting wpa_supplicant..."));
        QApplication::processEvents();
        QProcess::execute("/usr/sbin/wpa_supplicant -Dwext -iwlan0 -c/etc/wpa_supplicant.conf -B");
        QProcess::execute("/usr/sbin/wpa_cli scan");
        _timer.start(1500);
        ui->passEdit->setFocus();
    }
    else
    {
        QMessageBox::critical(this, tr("No wifi device found"), tr("No wifi USB device connected, or your device may not be supported"), QMessageBox::Close);
    }
}

WifiDialog::~WifiDialog()
{
    delete ui;
}

void WifiDialog::pollScanResults()
{
    if (!_proc)
    {
        _proc = new QProcess(this);
        connect(_proc, SIGNAL(finished(int)), SLOT(processScanResults(int)));
        _proc->start("/usr/sbin/wpa_cli scan_results");
    }
}

void WifiDialog::processScanResults(int exitCode)
{
    if (!_proc)
        return;

    if (exitCode == 0)
    {
        QList<QByteArray> lines = _proc->readAll().split('\n');
        if (lines.count()>2)
        {
            lines.removeFirst(); lines.removeFirst();
            foreach (QByteArray line, lines)
            {
                QList<QByteArray> parts = line.split('\t');
                /* bssid / frequency / signal level / flags / ssid */

                if (parts.count() == 5)
                {
                    QByteArray ssid = parts[4];

                    if (!ssid.isEmpty() && !_ssids.contains(ssid))
                    {
                        _ssids.append(ssid);
                        new QListWidgetItem(QIcon(":/icons/network_wireless.png"), ssid, ui->networkList);
                        if (ui->networkList->count() == 1)
                            ui->networkList->setCurrentRow(0);
                    }
                }
            }
        }
    }
    _proc->deleteLater();
    _proc = NULL;
}

/*
void WifiDialog::infoAvailable()
{
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(false);
    ui->networkList->clear();

    QList<QNetworkConfiguration> list = _man.allConfigurations();
    foreach (QNetworkConfiguration c, list)
    {
        if (c.bearerType() != c.BearerEthernet && c.bearerType() != c.BearerUnknown)
        {
            //qDebug() << c.name() << c.bearerTypeName() << c.type() << c.state();
            ui->networkList->addItem( c.name() );
        }
    }
}
*/



void WifiDialog::on_networkList_currentRowChanged(int)
{
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(true);
}

void WifiDialog::accept()
{
    QProcess::execute("/usr/bin/killall wpa_supplicant");
    QProgressDialog qpd(tr("Connecting to access point..."), QString(),0,0, this);
    qpd.show();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    QFile f("/etc/wpa_supplicant.conf");
    f.open(f.WriteOnly);
    f.write(
        "ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev\n"
        "ap_scan=1\n\n"
        "network={\n"
        "\tssid=\""+ui->networkList->currentItem()->text().toAscii()+"\"\n"
        "\tpsk=\""+ui->passEdit->text().toAscii()+"\"\n"
        "}\n"
    );
    f.close();
    QProcess::execute("/usr/sbin/wpa_supplicant -Dwext -iwlan0 -c/etc/wpa_supplicant.conf -B");

    /* Ugly workaround: sleep a second to give slow wifi devices some time. */
    QTime sleepUntil = QTime::currentTime().addSecs(1);
    while( QTime::currentTime() < sleepUntil )
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents, 100);

    /* Connection is successful if we get a DHCP lease
     * TODO: support static network configurations. Could ARP ping gateway to check connection */
    if ( QProcess::execute("/sbin/udhcpc -n -i wlan0") != 0 )
    {
        qpd.setLabelText(tr("Second try... Connecting to access point..."));
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

        if (QProcess::execute("/sbin/udhcpc -n -i wlan0") != 0 )
        {
            qpd.hide();
            QMessageBox::critical(this, tr("Error connecting"), tr("Error connecting or obtaining IP-address. Check settings."), QMessageBox::Ok);
            return;
        }
    }

    /* Everything ok. Copy wpa_supplicant.conf to boot partition */
    QFile::copy("/etc/wpa_supplicant.conf", "/boot/wpa_supplicant.conf");
    QDialog::accept();
}
