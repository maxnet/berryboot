#ifndef DRIVEFORMATTHREAD_H
#define DRIVEFORMATTHREAD_H

/* Berryboot -- drive format and partitioning thread
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

#include <QThread>
#include "installer.h"

class DriveFormatThread : public QThread
{
    Q_OBJECT
public:
    /*
     * Constructor
     *
     * - drive: e.g. mmcblk0
     * - filesystem: ext4 or btrfs
     * - Installer: reference to Installer object
     * - bootdev: boot device (defaults to mmcblk0p1)
     * - initializedata: initialize data partition (defaults to true)
     */
    explicit DriveFormatThread(const QString &drive, const QString &filesystem, Installer *i, QObject *parent = 0, const QString &bootdev = "mmcblk0p1", bool initializedata = true, bool password = false);
    void setPassword(const QByteArray &password);
    QString drive();
    QString bootdev();
    QString datadev();
    
signals:
    void error(const QString &msg);
    void statusUpdate(const QString &msg);
    void completed();
    
protected:
    QString _dev, _datadev, _bootdev, _fs;
    bool _reformatBoot, _iscsi, _initializedata, _password;
    Installer *_i;

    virtual void run();
    bool zeroMbr();
    bool installUbootSPL();
    bool partitionDrive();
    bool formatBootPartition();
    bool formatDataPartition();
    bool isBtrfs();
};

#endif // DRIVEFORMATTHREAD_H
