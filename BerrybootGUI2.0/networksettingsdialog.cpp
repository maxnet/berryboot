/* Berryboot -- proxy server settings dialog
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


#include "networksettingsdialog.h"
#include "ui_networksettingsdialog.h"
#include "installer.h"
#include <QSettings>
#include <QProgressDialog>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QDebug>

#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netdb.h>

NetworkSettingsDialog::NetworkSettingsDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NetworkSettingsDialog),
    _i(i)
{
    ui->setupUi(this);

    /* IP settings tab */
    QValidator *ipvalidator = new QRegExpValidator( QRegExp("0*(2(5[0-5]|[0-4]\\d)|1?\\d{1,2})(\\.0*(2(5[0-5]|[0-4]\\d)|1?\\d{1,2})){3}"), this);
    ui->ipEdit->setValidator(ipvalidator);
    ui->gwEdit->setValidator(ipvalidator);
    _setCurrentNetInformation();

    /* Proxy server tab */
    QValidator *val = new QIntValidator(this);
    ui->proxyportEdit->setValidator(val);
    QSettings *s = _i->settings();
    s->beginGroup("proxy");
    ui->proxyhostEdit->setText(s->value("hostname").toString());
    ui->proxyportEdit->setText(QString::number(s->value("port", 8080).toInt()));
    ui->proxyuserEdit->setText(s->value("user").toString());
    ui->proxypassEdit->setText(QByteArray::fromBase64(s->value("password").toByteArray()));
    s->endGroup();

    /* Repo tab */
    s->beginGroup("repo");
    ui->repoEdit->setText(s->value("url").toString());
    ui->repoUserEdit->setText(s->value("user").toString());
    ui->repoPassEdit->setText(QByteArray::fromBase64(s->value("password").toByteArray()));
    s->endGroup();
}

NetworkSettingsDialog::~NetworkSettingsDialog()
{
    delete ui;
}

void NetworkSettingsDialog::accept()
{
    /* Save network configuration settings */
    QByteArray newParam, oldParam = currentIPparam();
    QByteArray ip      = ui->ipEdit->text().toLatin1();
    QByteArray netmask = ui->netmaskCombo->currentText().toLatin1();
    QByteArray gateway = ui->gwEdit->text().toLatin1();
    QByteArray iface   = ui->interfaceCombo->currentText().toLatin1();
    QByteArray dns     = "8.8.8.8";
    bool staticConfig  = ui->staticRadio->isChecked();

    if (staticConfig)
    {
        newParam = "ipv4="+ip+"/"+netmask+"/"+gateway;
        if (iface != "eth0")
            newParam += "/"+iface;
    }

    if (oldParam != newParam)
    {
        /* Parameters have changed, update configuration files */

        QProgressDialog qpd(tr("Configuring network interface..."), QString(),0,0, this);
        qpd.show();
        QApplication::processEvents();

        QByteArray ifaces = "auto "+iface+"\n";

        if (staticConfig)
        {
            ifaces += "iface "+iface+" inet static\n"
                    "\taddress "+ip+"\n"
                    "\tnetmask "+netmask+"\n"
                    "\tgateway "+gateway+"\n"
                    "\tup echo 'nameserver "+dns+"' > /etc/resolv.conf\n";
        }
        else
        {
            ifaces += "iface "+iface+" inet dhcp\n";
        }

        QProcess::execute("/sbin/ifdown -a");

        QFile f("/etc/network/interfaces");
        f.open(f.WriteOnly);
        f.write(ifaces);
        f.close();

        QProcess::execute("/sbin/ifup -a");

        /* Update /etc/network/network/interfaces shared by distributions */
        if (QFile::exists("/mnt/shared/etc"))
        {
            if (staticConfig)
            {
                QDir dir;
                dir.mkdir("/mnt/shared/etc/network");
                ifaces = "# Static network configuration handled by Berryboot\n"
                        "iface "+iface+" inet manual\n\n"
                        "auto lo\n"
                        "iface lo inet loopback\n";
                f.setFileName("/mnt/shared/etc/network/interfaces");
                f.open(f.WriteOnly);
                f.write(ifaces);
                f.close();

                f.setFileName("/mnt/shared/etc/resolv.conf");
                f.open(f.WriteOnly);
                f.write("nameserver "+dns);
                f.close();
            }
            else
            {
                /* Default /etc/network/interfaces file should cover DHCP */
                QFile::remove("/mnt/shared/etc/network/interfaces");
                QFile::remove("/mnt/shared/etc/resolv.conf");
            }
        }

        /* Replace ipv4 parameter in cmdline.txt and uEnv.txt */
        if (!newParam.isEmpty())
            newParam = " "+newParam;
        oldParam = "ipv4="+oldParam;

        f.setFileName("/boot/cmdline.txt");
        f.open(f.ReadOnly);
        QByteArray line = f.readAll();
        line.replace(oldParam, "");
        line = line.trimmed();
        line += newParam;
        f.close();
        f.open(f.WriteOnly);
        f.write(line);
        f.close();
        _i->setBootoptions(line);

        f.setFileName("/boot/uEnv.txt");
        f.open(f.ReadOnly);
        line = f.readAll();
        line.replace(oldParam, "");
        line = line.trimmed();
        line += newParam+"\n";
        f.close();
        f.open(f.WriteOnly);
        f.write(line);
        f.close();
    }

    /* Save proxy settings */
    QString host = ui->proxyhostEdit->text().trimmed();
    QSettings *s = _i->settings();
    s->beginGroup("proxy");

    if (host.isEmpty())
    {
        s->remove("");
    }
    else
    {
        s->setValue("type", 4 /*QNetworkProxy::HttpCachingProxy*/);
        s->setValue("hostname", host);
        s->setValue("port", ui->proxyportEdit->text().toInt());
        s->setValue("user", ui->proxyuserEdit->text());
        s->setValue("password", ui->proxypassEdit->text().toLatin1().toBase64());
    }

    s->endGroup();

    s->beginGroup("repo");
    if (ui->repoEdit->text().isEmpty())
        s->remove("url");
    else
        s->setValue("url", ui->repoEdit->text() );
    if (ui->repoUserEdit->text().isEmpty())
        s->remove("user");
    else
        s->setValue("user", ui->repoUserEdit->text() );
    if (ui->repoPassEdit->text().isEmpty())
        s->remove("password");
    else
        s->setValue("password", ui->repoPassEdit->text().toLatin1().toBase64());

    s->endGroup();
    s->sync();

    QDialog::accept();
}

/* Get currently in use IP, netmask & gateway settings
 * We could use Qt's functions instead to get this information, but that would create a dependency on QtNetwork */
void NetworkSettingsDialog::_setCurrentNetInformation()
{
    struct ifaddrs *ifaddr, *ifa;
    int family;
    char ipstr[32], netmaskstr[32];
    QStringList ifaces;
    bool haveIp = false;

    if (getifaddrs(&ifaddr) == -1)
        return;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;
        QString iface = ifa->ifa_name;
        if (iface == "lo")
            continue;

        if (!ifaces.contains(iface))
            ifaces.append(iface);

        if (family == AF_INET && !haveIp)
        {
            if  (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ipstr, sizeof(ipstr), NULL, 0, NI_NUMERICHOST) == 0
              && getnameinfo(ifa->ifa_netmask, sizeof(struct sockaddr_in), netmaskstr, sizeof(netmaskstr), NULL, 0, NI_NUMERICHOST) == 0)
            {
//                qDebug() << iface << ipstr << netmaskstr;
                ui->ipEdit->setText(ipstr);
                ui->netmaskCombo->setCurrentIndex(ui->netmaskCombo->findText(netmaskstr));
                haveIp = true;
            }
        }
    }
    freeifaddrs(ifaddr);
    ifaces.sort();
    ui->interfaceCombo->addItems(ifaces);

    QString defaultInterface;
    ui->gwEdit->setText(getDefaultGateway(defaultInterface));

    if (ui->interfaceCombo->count())
    {
        int ifIdx = ui->interfaceCombo->findText(defaultInterface);

        if (ifIdx != -1)
            ui->interfaceCombo->setCurrentIndex(ifIdx);
        else
            ui->interfaceCombo->setCurrentIndex(0);
    }

    if (currentIPparam().isEmpty())
        ui->dhcpRadio->setChecked(true);
    else
        ui->staticRadio->setChecked(true);
}

QString NetworkSettingsDialog::getDefaultGateway(QString &interface)
{
    /* Probably better to use ioctl()s but this will do for now */

    /* Format of /proc/net/route
    Iface   Destination     Gateway         Flags   RefCnt  Use     Metric  Mask            MTU     Window  IRTT
    eth0    00000000        0158A8C0        0003    0       0       0       00000000        0       0       0 */

    QFile f("/proc/net/route");
    f.open(f.ReadOnly);
    QList<QByteArray> lines = f.readAll().split('\n');
    f.close();

    foreach (QByteArray line, lines)
    {
        QList<QByteArray> parts = line.split('\t');

        if (parts.count() > 10 && parts[1] == "00000000" && parts[2].length() == 8) /* default IPv4 gateway */
        {
            QByteArray gwhex = parts[2];
            interface = parts[0];
            return QString::number(gwhex.mid(6,2).toInt(NULL, 16))+"."+
                   QString::number(gwhex.mid(4,2).toInt(NULL, 16))+"."+
                   QString::number(gwhex.mid(2,2).toInt(NULL, 16))+"."+
                   QString::number(gwhex.mid(0,2).toInt(NULL, 16));
        }
    }

    return "";
}

/* Get IP auto-configuration parameter */
QByteArray NetworkSettingsDialog::currentIPparam()
{
    QFile f("/boot/cmdline.txt");
    f.open(f.ReadOnly);
    QByteArray cmdline = f.readAll();
    f.close();

    int pos = cmdline.indexOf("ipv4=");
    if (pos == -1)
    {
        return "";
    }
    else
    {
        int end = cmdline.indexOf(' ', pos+5);
        if (end != -1)
            end = end-pos-5;
        return cmdline.mid(pos+5, end);
    }
}


void NetworkSettingsDialog::on_dhcpRadio_toggled(bool checked)
{
    ui->staticGroup->setEnabled(!checked);
}
