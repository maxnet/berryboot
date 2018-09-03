/* Berryboot -- wifi country detector
 *
 * Copyright (c) 2018, Floris Bos
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

#include "wificountrydetector.h"
#include "installer.h"
#include <QProcess>
#include <QMap>
#include <QRegExp>
#include <QTime>

WifiCountryDetector::WifiCountryDetector(Installer *i, QObject *parent) :
    QThread(parent), _i(i)
{
}

void WifiCountryDetector::run()
{
    QProcess p;
    QMap<QByteArray,int> countries;
    QRegExp rx("Country: ([A-Z]{2})");

    emit statusUpdate("Loading drivers");
    _i->loadDrivers();
    emit statusUpdate("Waiting for wifi device to appear");

    QTime t;
    t.start();
    while (!QFile::exists("/sys/class/net/wlan0"))
    {
        if (t.elapsed() > 4000)
        {
            // Timeout after 4 seconds
            emit detectionFailed();
            return;
        }

        QThread::msleep(100);
    }

    emit statusUpdate("Enabling wlan0 interface");
    QProcess::execute("/sbin/ifconfig wlan0 up");
    emit statusUpdate("Auto-detecting wifi country");
    p.start("/usr/sbin/iw wlan0 scan passive");
    p.waitForFinished();

    while (p.canReadLine())
    {
        QByteArray line = p.readLine();
        if (rx.indexIn(line) != -1)
        {
            QByteArray country = rx.cap(1).toLatin1();
            if (country != "EU")
            {
                if (countries.contains(country))
                    countries[country]++;
                else
                    countries.insert(country, 1);
            }
        }
    }

    if (countries.count())
    {
        QByteArray country;
        int highestAp = 0;

        foreach (QByteArray c, countries.keys())
        {
            int ap = countries[c];

            if (ap > highestAp)
            {
                country = c;
                highestAp = ap;
            }
        }

        emit countryDetected(country, highestAp);
    }
    else
    {
        emit detectionFailed();
    }
}
