#ifndef DISKDIALOG_H
#define DISKDIALOG_H

/* Berryboot -- disk selection dialog
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

#include <QDialog>
#include <QTimer>

namespace Ui {
class DiskDialog;
}
class QProgressDialog;
class Installer;

class DiskDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit DiskDialog(Installer *i, QWidget *parent = 0);
    ~DiskDialog();
    bool usbBoot();
    
protected:
    Ui::DiskDialog *ui;
    int _devlistcount;
    QTimer _pollDisksTimer;
    QProgressDialog *_qpd;
    Installer *_i;
    bool _usbboot;

    /*
     * Test if drive has an existing Berryboot installation
     */
    bool hasExistingBerryboot(const QString &drive);

protected slots:
    /*
     * Populate GUI widget with available drives
     */
    void populateDrivelist();
    /*
     * Called periodicly to poll for USB drive insertion
     */
    void pollForNewDisks();

    /*
     * Called when formatting the drive is complete
     */
    void onFormatComplete();

    /*
     * Called on format error
     */
    void onError(const QString &error);

private slots:
    /*
     * Called when "format" button has been clicked by user
     */
    void on_formatButton_clicked();
    void on_driveList_currentRowChanged(int currentRow);
    void on_filesystemCombo_currentIndexChanged(const QString &arg1);
};

#endif // DISKDIALOG_H
