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
#include <QRegExp>

WifiDialog::WifiDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WifiDialog),
    _i(i),
    _proc(NULL),
    _firstPoll(true)
{
    ui->setupUi(this);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(pollScanResults()));

    QProgressDialog qpd(tr("Loading drivers"), QString(), 0, 0, this);
    qpd.show();
    QApplication::processEvents();
    _i->loadDrivers();
    qpd.hide();

    QFile conffile("/boot/wpa_supplicant.conf");
    if (conffile.exists())
    {
        conffile.open(conffile.ReadOnly);
        _config = conffile.readAll();
        conffile.close();

        QRegExp rp("country=\"?([A-Za-z]{2})\"?");
        if (rp.indexIn(_config) != -1)
        {
            _country = rp.cap(1).toLatin1();
        }
    }

    /* Wait up to 4 seconds for wifi device to appear */
    QTime t;
    t.start();
    while (t.elapsed() < 4000 && !QFile::exists("/sys/class/net/wlan0") )
    {
        QApplication::processEvents(QEventLoop::WaitForMoreEvents, 250);
    }

    if ( QFile::exists("/sys/class/net/wlan0") )
    {
        qpd.setLabelText(tr("Starting wpa_supplicant..."));
        QApplication::processEvents();
        QProcess::execute("/usr/sbin/wpa_supplicant -s -Dnl80211,wext -iwlan0 -c/etc/wpa_supplicant.conf -B");
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

void WifiDialog::setCountry(const QByteArray &country)
{
    _country = country;
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
                    int level = parts[2].toInt();
                    QString icon;

                    if (!ssid.isEmpty() && !_ssids.contains(ssid))
                    {
                        _ssids.append(ssid);

                        if (level > -50)
                            icon = ":/icons/network-wireless-connected-100.png";
                        else if (level > -60)
                            icon = ":/icons/network-wireless-connected-75.png";
                        else if (level > -70)
                            icon = ":/icons/network-wireless-connected-50.png";
                        else if (level > -80)
                            icon = ":/icons/network-wireless-connected-25.png";
                        else
                            icon = ":/icons/network-wireless-disconnected.png";

                        new QListWidgetItem(QIcon(icon), ssid, ui->networkList);
                        if (ui->networkList->count() == 1)
                            ui->networkList->setCurrentRow(0);
                    }
                }
            }
        }
    }
    _proc->deleteLater();
    _proc = NULL;

    if (_firstPoll)
    {
        /* Check if we are already connected to a SSID */
        QProcess p;
        p.start("/usr/sbin/wpa_cli status");
        p.waitForFinished(1000);
        QByteArray s = p.readAll();
        if (s.contains("wpa_state=COMPLETED"))
        {
            qDebug() << "has connection";
            QRegExp r("\nssid=([^\n]+)");
            if ( r.indexIn(s) != -1 )
            {
                QString currentSSID = r.cap(1);

                QList<QListWidgetItem *> items = ui->networkList->findItems(currentSSID, Qt::MatchExactly);
                if (items.count())
                {
                    ui->networkList->setCurrentItem(items.first());

                    /* Fetch password from wpa_supplicant.conf */
                    QRegExp rp("psk=\"([^\"]+)\"");
                    if (rp.indexIn(_config) != -1)
                    {
                        ui->passEdit->setText(rp.cap(1));
                    }
                }
            }
        }
        _firstPoll = false;
    }
}

void WifiDialog::on_networkList_currentRowChanged(int)
{
    ui->buttonBox->button(ui->buttonBox->Ok)->setEnabled(true);
}

void WifiDialog::accept()
{
    if (QProcess::execute("/usr/bin/killall wpa_supplicant") == 0)
    {
        QTime sleepUntil = QTime::currentTime().addMSecs(500);
        while( QTime::currentTime() < sleepUntil )
            QApplication::processEvents(QEventLoop::WaitForMoreEvents, 100);
    }

    if (ui->networkList->currentRow() == 0)
    {
        /* Wired connection */
        if (QFile::exists("/boot/wpa_supplicant.conf"))
            QFile::remove("/boot/wpa_supplicant.conf");
        if (QFile::exists("/mnt/shared/etc/wpa_supplicant/wpa_supplicant.conf"))
            QFile::remove("/mnt/shared/etc/wpa_supplicant/wpa_supplicant.conf");

        QDialog::accept();
        return;
    }

    QProgressDialog qpd(tr("Connecting to access point..."), QString(),0,0, this);
    qpd.show();
    QApplication::processEvents();

    QFile f("/etc/wpa_supplicant.conf");
    f.open(f.WriteOnly);
    if (!_country.isEmpty())
        f.write("country="+_country+"\n");
    f.write(
        "ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev\n"
        "ap_scan=1\n\n"
        "update_config=1\n"
        "network={\n"
        "\tssid=\""+ui->networkList->currentItem()->text().toLatin1()+"\"\n"
        "\tpsk=\""+ui->passEdit->text().toLatin1()+"\"\n"
        "}\n"
    );
    f.close();
    QProcess::execute("/usr/sbin/wpa_supplicant -s -Dnl80211,wext -iwlan0 -c/etc/wpa_supplicant.conf -B");

    /* Ugly workaround: sleep a second to give slow wifi devices some time. */
    QTime sleepUntil = QTime::currentTime().addSecs(1);
    while( QTime::currentTime() < sleepUntil )
        QApplication::processEvents(QEventLoop::WaitForMoreEvents, 100);

    /* Connection is successful if we get a DHCP lease
     * TODO: support static network configurations. Could ARP ping gateway to check connection */
    QProcess proc;
    connect(&proc, SIGNAL(finished(int)), &qpd, SLOT(hide()));
    proc.start("/sbin/udhcpc -n -i wlan0 -O mtu -t 5 -T 2");
    qpd.exec();

    if (proc.exitCode() != 0 )
    {
        qpd.hide();
        QMessageBox::critical(this, tr("Error connecting"), tr("Error connecting or obtaining IP-address. Check settings."), QMessageBox::Ok);
        return;
    }

    /* Everything ok. Copy wpa_supplicant.conf to boot partition */
    if (QFile::exists("/boot/wpa_supplicant.conf"))
        QFile::remove("/boot/wpa_supplicant.conf");
    QFile::copy("/etc/wpa_supplicant.conf", "/boot/wpa_supplicant.conf");
    QDialog::accept();
}

void WifiDialog::on_networkList_itemClicked()
{
    ui->passEdit->setFocus();
}
