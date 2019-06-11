#ifndef BOOTMENUDIALOG_H
#define BOOTMENUDIALOG_H

/* Berryboot -- boot menu dialog
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
#include <QModelIndex>
#include <QProcess>

namespace Ui {
class BootMenuDialog;
}
class Installer;

class BootMenuDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit BootMenuDialog(Installer *i, QWidget *parent = 0);
    ~BootMenuDialog();
    virtual bool eventFilter(QObject *obj, QEvent *event);

protected:
    /* Utility functions copied from old BerrybootUI application */

    /*
     * Boot file system image specified by name
     */
    void bootImage(const QString &name);
    /*
     * Start the installer
     */
    void startInstaller();
    /*
     * Load modules, and mount iSCSI disk
     */
    void startISCSI();
    /*
     * Load specified module
     */
    void loadModule(const QByteArray &name);
    /*
     * Load Allwinner A10 modules
     */
    void initializeA10();
    /*
     * Load data from file
     */
    QByteArray file_get_contents(const QString &filename);
    /*
     * Save data to file
     */
    void file_put_contents(const QString &filename, const QByteArray &data);
    /*
     * Get /proc/cmdline
     */
    QByteArray getBootOptions();
    /*
     * Mount data partition
     */
    bool mountDataPartition(const QByteArray &dev, bool rw = false);
    /*
     * Wait for disk device to become available
     */
    bool waitForDevice(QByteArray &dev);
    /*
     * Mount boot partition
     */
    void mountSystemPartition();
    /*
     * Unmount boot partition
     */
    void umountSystemPartition();
    /*
     * Return current memory split in MB
     * In the case of a 512 MB model, return 256 MB equivalent with same gpu_mem
     */
    int currentMemsplit();
    /*
     * Return the memory split the image prefers
     */
    int imageNeedsMemsplit(const QString &name);
    /*
     * Return gpu_mem config value for memsplit
     */
    QByteArray memsplitParameter(int memsplit);
    /*
     * Reboot system
     */
    void reboot();
    /*
     * Halt the system
     */
    void halt();
    /*
     * Sleep miliseconds
     */
    void processEventSleep(int ms);
    /*
     * False if using CCM
     */
    bool memsplitsEnabled();
    /*
     * Called when the user wants to close the window
     * We disallow that (ignore the event)
     */
    virtual void closeEvent(QCloseEvent *ev);
    /*
     * Ask for password in text console
     */
    void askLuksPassword(const QString &datadev);
    /*
     * Start SSH daemon (if ssh_authorized_key boot parameter is present)
     */
    void startSSHserverIfEnabled();
    /*
     * Stop SSH daemon
     */
    void stopSSHserver();
    /*
     * Wait for remount RW
     */
    void waitForRemountRW();
    /*
     * Show locale settings dialog
     */
    void reconfigureLocale();

    Ui::BootMenuDialog *ui;
    Installer *_i;
    int _countdown;
    QTimer _countdownTimer;
    QProcess _remountproc;

protected slots:
    void on_bootButton_clicked();
    void on_settingsButton_clicked();
    void on_poweroffButton_clicked();
    void on_list_activated(const QModelIndex &index);
    void autoBootTimeout();
    void stopCountdown();
    void initialize();
};

#endif // BOOTMENUDIALOG_H
