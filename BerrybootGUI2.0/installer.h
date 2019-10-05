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

#define BERRYBOOT_VERSION  "v2.908"
#define SIZE_BOOT_PART  /* 63 */ 127

class QSettings;

class Installer : public QObject
{
    Q_OBJECT
public:
    explicit Installer(QObject *parent = 0);
    
    bool saveBootFiles();
    bool restoreBootFiles();
    int sizeofBootFilesInKB();
    void initializeDataPartition(const QString &dev);
    bool mountSystemPartition();
    bool umountSystemPartition();
    bool networkReady();
    bool eth0Up();
    QByteArray datadev();
    QByteArray bootdev();
    QByteArray findBootPart();
    double availableDiskSpace(const QString &path = "/mnt");
    double diskSpaceInUse(const QString &path = "/mnt");
    void reboot();
    void prepareDrivers();
    void cleanupDrivers();
    void loadCryptoModules();
    void loadSoundModule(const QByteArray &channel);
    void loadFilesystemModule(const QByteArray &fs);

    void setSkipConfig(bool skip);
    void setKeyboardLayout(const QString &layout);
    void setTimezone(const QString &tz);
    QString timezone() const;
    QString keyboardlayout() const;
    bool skipConfig() const;
    void setDisableOverscan(bool disabled);
    bool disableOverscan() const;
    void setFixateMAC(bool fix);
    bool fixateMAC() const;
    void setSound(const QByteArray &sound);
    QByteArray sound() const;

    QMap<QString,QString> listInstalledImages();
    QString imageFilenameToFriendlyName(const QString &name);

    QString getDefaultImage();
    void setDefaultImage(const QString &name);
    void renameImage(const QString &oldname, const QString &newName);
    void deleteImage(const QString &name);
    void deleteUserChanges(const QString &name);
    void cloneImage(const QString &oldname, const QString &newName, bool clonedata);

    bool isSquashFSimage(QFile &f);
    void enableCEC();
    QSettings *settings();
    bool hasSettings();
    bool isMemsplitHandlingEnabled();
    QByteArray cpuinfo();
    bool hasOverscanSettings();
    bool hasDynamicMAC();
    QByteArray macAddress();
    void switchConsole(int ttynr);
    QByteArray bootoptions();
    QByteArray bootParam(const QByteArray &name);
    void setBootoptions(const QByteArray &newOptions);
    QByteArray model();
    bool supportsUSBboot();
    QString iscsiDevice();
    bool isPxeBoot();
    QByteArray uuidOfDevice(QByteArray device);
    QByteArray getDeviceByLabel(const QByteArray &label);
    QByteArray getDeviceByUuid(const QByteArray &uuid);
    QByteArray getPartitionByLabel(const QByteArray &label);
    QByteArray getPartitionByUuid(const QByteArray &uuid);

public slots:
    void startNetworking();
    void startWifi();
    void startNetworkInterface();
    void loadDrivers();

protected:
    QString _keyboardlayout, _timezone;
    bool _disableOverscan, _fixMAC, _ethup, _skipConfig;
    QSettings *_settings;
    QByteArray _bootoptions, _sound, _bootdev;

    void log_error(const QString &msg);


protected slots:
    void ifupFinished(int exitCode);
    void onKeyPress(int key);
    void wifiStarted(int rc);

signals:
    void networkInterfaceUp();
    void error(const QString &msg);

    
public slots:
    
};

#endif // INSTALLER_H
