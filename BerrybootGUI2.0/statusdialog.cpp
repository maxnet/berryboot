/* Berryboot -- Connection status dialog
 *
 * Copyright (c) 2016, Floris Bos
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

#include "statusdialog.h"
#include "ui_statusdialog.h"
#include "wifidialog.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QRegExp>
#include <QDebug>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkInterface>
#include <wpa_ctrl.h>

StatusDialog::StatusDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StatusDialog),
    _i(i),
    _monitorConn(NULL),
    _controlConn(NULL),
    _ctrlPath("/var/run/wpa_supplicant/wlan0"),
    _notifier(NULL),
    _isUp(false)
{
    setAttribute(Qt::WA_ShowWithoutActivating);
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_QuitOnClose, false);
    setMask(topRoundedRect(rect(), 10));

    ui->wifiCombo->installEventFilter(this);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(pollCtrlDir()));
    _timer.start(500);
}

QRegion StatusDialog::topRoundedRect(const QRect& rect, int r)
{
    QRegion region;

    region += rect.adjusted(r, 0, -r, 0);
    region += rect.adjusted(0, r, 0, -r);
    QRect corner(rect.topLeft(), QSize(r*2, r*2));
    region += QRegion(corner, QRegion::Ellipse);
    corner.moveTopRight(rect.topRight());
    region += QRegion(corner, QRegion::Ellipse);
    corner.moveBottomLeft(rect.bottomLeft());
    region += QRegion(corner, QRegion::Rectangle);
    corner.moveBottomRight(rect.bottomRight());
    region += QRegion(corner, QRegion::Rectangle);

    return region;
}

StatusDialog::~StatusDialog()
{
    if (_monitorConn)
    {
        wpa_ctrl_detach(_monitorConn);
        wpa_ctrl_close(_monitorConn);
        wpa_ctrl_close(_controlConn);
    }

    delete ui;
}

void StatusDialog::connectToWpa()
{
    qDebug() << "(re)connect to wpa";

    if (_monitorConn)
    {
        delete _notifier;
        wpa_ctrl_detach(_monitorConn);
        wpa_ctrl_close(_monitorConn);
        wpa_ctrl_close(_controlConn);
        _monitorConn = _controlConn = NULL;
    }

    _monitorConn = wpa_ctrl_open(_ctrlPath.constData());
    if (_monitorConn)
    {
        _notifier = new QSocketNotifier(wpa_ctrl_get_fd(_monitorConn), QSocketNotifier::Read, this);
        connect(_notifier, SIGNAL(activated(int)), SLOT(readMessages()));
        wpa_ctrl_attach(_monitorConn);
        _controlConn = wpa_ctrl_open(_ctrlPath.constData());
        qDebug() << "WPA control connect successful";
    }
    else
    {
        qDebug() << "WPA control connect unsuccessful";
    }

    updateStatus();
}

void StatusDialog::readMessages()
{
    char buf[1024];
    size_t len;

    while (_monitorConn && wpa_ctrl_pending(_monitorConn) > 0) {
        len = sizeof(buf) - 1;

        if (wpa_ctrl_recv(_monitorConn, buf, &len) == 0)
        {
            QByteArray msg(buf, len);

            if (msg.startsWith('<') && msg.contains('>'))
            {
                /* Skip log level */
                msg = msg.mid(msg.indexOf('>')+1);
            }

            qDebug() << "WPA event:" << msg;

            if (msg.startsWith(WPA_EVENT_CONNECTED)
//                    || msg.startsWith(WPA_EVENT_DISCONNECTED)
                    || msg.startsWith(WPA_EVENT_ASSOC_REJECT)
                    || msg.startsWith(WPA_EVENT_AUTH_REJECT) )
            {
                updateStatus();
            }
            else if (msg.startsWith(WPA_EVENT_TERMINATING))
            {
                delete _notifier;
                //wpa_ctrl_detach(_monitorConn);
                wpa_ctrl_close(_monitorConn);
                wpa_ctrl_close(_controlConn);
                _monitorConn = _controlConn = NULL;
                _isUp = false;
            }
        }
    }
}

void StatusDialog::updateStatus()
{
    QString status = "No connection";
    QString icon   = ":/icons/remove.png";
    QString ipstr  = ip();

    if (_controlConn)
    {
        char buf[1024];
        size_t len = sizeof(buf) - 1;

        qDebug() << "Requesting WPA status";

        if (wpa_ctrl_request(_controlConn, "STATUS", 6, buf, &len, NULL) == 0)
        {
            QByteArray reply(buf, len);

            qDebug() << "WPA status" << reply;

            QRegExp r("\nssid=([^\n]+)");
            if ( r.indexIn(reply) != -1 )
            {
                QString ssid = r.cap(1);
                status = ssid;

                if (!ipstr.isEmpty())
                {
                    status += " ["+ipstr+"]";
                }
            }

            if (reply.contains("wpa_state=COMPLETED"))
            {
                icon = ":/icons/network-wireless-connected-100.png";
            }
        }
    }
    else if (!ipstr.isEmpty())
    {
        status = "Wired connection ["+ipstr+"]";
        icon   = ":/icons/network_wired.png";
    }

    ui->wifiCombo->setItemIcon(0, QIcon(icon));
    ui->wifiCombo->setItemText(0, status);
}

/* Check every half a second if wpa_supplicant has (re)started,
   or a (wired) connection has been made */
void StatusDialog::pollCtrlDir()
{
    QFileInfo fi(_ctrlPath);

    if (fi.exists() && fi.lastModified() != _lastmod)
    {
        _lastmod = fi.lastModified();
        connectToWpa();
    }
    else if (!_isUp && isNetworkUp() )
    {
        _isUp = true;
        updateStatus();
    }
}

bool StatusDialog::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QTimer::singleShot(1, this, SLOT(showWifiDialog()) );
        return true;
    }

    return false;
}

QString StatusDialog::ip()
{
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

    foreach (QHostAddress a, addresses)
    {
        if (a != QHostAddress::LocalHost && a.protocol() == QAbstractSocket::IPv4Protocol)
        {
            return a.toString();
        }
    }

    return QString();
}

bool StatusDialog::isNetworkUp()
{
    return !ip().isEmpty();
}

void StatusDialog::showWifiDialog()
{
    WifiDialog wd(_i);
    if (wd.exec() == wd.Accepted)
    {
        if (QFile::exists("/boot/wpa_supplicant.conf"))
        {
            if (QFile::exists("/mnt/shared/etc/wpa_supplicant/wpa_supplicant.conf"))
            {
                QFile::remove("/mnt/shared/etc/wpa_supplicant/wpa_supplicant.conf");
            }
            else
            {
                QDir dir;
                dir.mkdir("/mnt/shared/etc/wpa_supplicant");
            }
            QFile::copy("/boot/wpa_supplicant.conf", "/mnt/shared/etc/wpa_supplicant/wpa_supplicant.conf");
            QFile::setPermissions("/mnt/shared/etc/wpa_supplicant/wpa_supplicant.conf", QFile::ReadOwner | QFile::WriteOwner);
        }
    }
}
