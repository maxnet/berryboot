#ifndef MAINWINDOW_H
#define MAINWINDOW_H

/* Berryboot -- main boot menu editor window
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


#include <QMainWindow>
#include <QStringList>
#include <QModelIndex>

namespace Ui {
class MainWindow;
}
class Installer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(Installer *i, QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionExit_triggered();
    void on_actionAdd_OS_triggered();

    void copyOSfromUSB();
    void onCopyFailed();
    void onFormattingComplete();
    void onBackupComplete(int code);
    void cleanupUSBdevices();
    void mksquashfsFinished(int code);

    void on_actionEdit_triggered();
    void on_actionClone_triggered();
    void on_actionDelete_triggered();
    void on_actionSet_default_triggered();
    void on_list_currentRowChanged(int currentRow);

    void on_actionExport_triggered();

    void on_actionAdvanced_configuration_triggered();

    void on_actionConsole_triggered();

    void on_list_activated(const QModelIndex &);

    void on_actionSetPassword_triggered();

    void on_actionRepair_file_system_triggered();

    void on_actionRecover_triggered();

    void on_actionAdvMenu_toggled(bool arg1);

protected:
    Ui::MainWindow *ui;
    Installer *_i;
    QStringList partlist;

    bool scanUSBdevices(bool mountrw = false);
    QString externalSDcardDevice();
    void populate();
    void mksquashfs(QString imagename, QString destfileName, QStringList exclList, bool compress);

    virtual void closeEvent(QCloseEvent *event);
    void setButtonsEnabled(bool enable);
};

#endif // MAINWINDOW_H
