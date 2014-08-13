#ifndef NETWORKSETTINGSDIALOG_H
#define NETWORKSETTINGSDIALOG_H

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


#include <QDialog>

namespace Ui {
class NetworkSettingsDialog;
}
class Installer;

class NetworkSettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit NetworkSettingsDialog(Installer *i, QWidget *parent = 0);
    ~NetworkSettingsDialog();

public slots:
    virtual void accept();
    
protected:
    Ui::NetworkSettingsDialog *ui;
    Installer *_i;
    void _setCurrentNetInformation();
    QString getDefaultGateway(QString &interface);
    QByteArray currentIPparam();

private slots:
    void on_dhcpRadio_toggled(bool checked);
};

#endif // NETWORKSETTINGSDIALOG_H
