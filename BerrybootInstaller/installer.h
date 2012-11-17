#ifndef INSTALLER_H
#define INSTALLER_H

/* Berryboot -- installer utility class
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


#include <QObject>
#include <QProcess>
#include <QMap>
#include <QFile>

#define BERRYBOOT_VERSION  "v1.2f"

class Installer : public QObject
{
    Q_OBJECT
public:
    explicit Installer(QObject *parent = 0);
    
    bool saveBootFiles();
    bool restoreBootFiles();
    bool mountDataPartition(const QString &dev);
    void initializeDataPartition(const QString &dev);
    bool mountSystemPartition();
    void umountSystemPartition();
    void startNetworking();
    bool networkReady();
    QString datadev();
    double availableDiskSpace(const QString &path = "/mnt");
    void reboot();
    void loadDrivers();
    void startWifi();

    void setKeyboardLayout(const QString &layout);
    void setTimezone(const QString &tz);
    QString timezone() const;
    QString keyboardlayout() const;
    void setDisableOverscan(bool disabled);
    bool disableOverscan() const;

    QMap<QString,QString> listInstalledImages();
    QString imageFilenameToFriendlyName(const QString &name);

    QString getDefaultImage();
    void setDefaultImage(const QString &name);
    void renameImage(const QString &oldname, const QString &newName);
    void deleteImage(const QString &name);
    void cloneImage(const QString &oldname, const QString &newName, bool clonedata);

    bool isSquashFSimage(QFile &f);

protected:
    QString _keyboardlayout, _timezone;
    bool _disableOverscan;

    void log_error(const QString &msg);


protected slots:
    void ifupFinished(int exitCode);

signals:
    void networkInterfaceUp();
    void error(const QString &msg);

    
public slots:
    
};

#endif // INSTALLER_H
