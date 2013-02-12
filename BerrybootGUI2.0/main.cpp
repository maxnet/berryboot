/*
Copyright (c) 2012, Floris Bos
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QApplication>
#include "mainwindow.h"
#include "diskdialog.h"
#include "installer.h"
#include "adddialog.h"
#include "logviewer.h"
#include "localedialog.h"
#include "logindialog.h"
#include "wifidialog.h"
#include "bootmenudialog.h"
#include <QDebug>
#include <QStyle>
#include <QDesktopWidget>
#include <QImage>
#include <QSettings>
#include <QFile>
#include <QScreen>

#ifdef Q_WS_QWS
#include <QWSServer>
#endif

#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifdef Q_WS_QWS
    if (QFile::exists(":/wallpaper.jpg"))
    {
        QRect dim = a.desktop()->availableGeometry();
        QWSServer::setBackground(QImage(":/wallpaper.jpg").scaled(dim.width(), dim.height()));
    }
    else
        QWSServer::setBackground(Qt::black);
#endif
    Installer i;
    i.enableCEC();

/*    if (system("syslogd") != 0) { qDebug() << "Error starting syslogd"; }
    if (system("klogd") != 0) { qDebug() << "Error starting klogd"; }
    if (system("/sbin/getty -L tty2 0 vt100 &") != 0) { qDebug() << "Error starting emergency holographic shell"; } */

    BootMenuDialog menu(&i);
    if (menu.exec() == menu.Rejected)
    {
        /* If menu.exec() returns rejected exit, otherwise show OS installer */

/*        if (system("killall getty") != 0) { qDebug() << "Error killing getty"; }
        if (system("killall syslogd") != 0) {qDebug() << "Error killing syslogd"; }
        if (system("killall klogd") != 0) {qDebug() << "Error killing klogd"; }*/
        return 0;
    }

    /* Check if installer is protected by password */
    if (i.hasSettings() && i.settings()->contains("berryboot/passwordhash"))
    {
        LoginDialog ld(&i);
        if (ld.exec() == ld.Rejected)
        {
            i.reboot();
            return 0;
        }
    }

    if (QFile::exists("/boot/wpa_supplicant.conf"))
        i.startWifi();

    if ( i.datadev().isEmpty() ) /* New installation */
    {
        if (system("/sbin/getty -L tty2 0 vt100 &") != 0) { qDebug() << "Error starting emergency holographic shell"; }

#ifdef Q_WS_QWS
        /* Write a message to serial console */
        QString serialdev;
        if (QFile::exists("/dev/ttyS0"))
            serialdev = "/dev/ttyS0";
        else if (QFile::exists("/dev/ttyAMA0"))
            serialdev = "/dev/ttyAMA0";

        if (!serialdev.isEmpty())
        {
            QFile f(serialdev);
            f.open(f.WriteOnly);
            f.write("Berryboot installer running ");
            if ( QScreen::instance()->classId() == QScreen::VNCClass)
                f.write("a headless VNC installation\n");
            else
                f.write("on display.\nIf you want a headless installation instead, append the option 'vncinstall' to cmdline.txt + uEnv.txt\n");
            f.close();
        }
#endif

        LocaleDialog ld(&i);
        ld.exec();
        DiskDialog w(&i);
        w.exec();
        AddDialog a(&i);
        a.exec();
    }

    MainWindow mw(&i);
    mw.setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, mw.size(), a.desktop()->availableGeometry()));
    mw.show();

    return a.exec();
}
