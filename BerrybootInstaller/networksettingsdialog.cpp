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


#include "networksettingsdialog.h"
#include "ui_networksettingsdialog.h"
#include <QtNetwork/QNetworkProxy>
#include <QDebug>

NetworkSettingsDialog::NetworkSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NetworkSettingsDialog)
{
    ui->setupUi(this);
    QValidator *val = new QIntValidator(this);
    ui->proxyportEdit->setValidator(val);

    QNetworkProxy proxy = QNetworkProxy::applicationProxy();

    if (proxy.type() != proxy.NoProxy)
    {
        ui->proxyhostEdit->setText(proxy.hostName());
        ui->proxyportEdit->setText(QString::number(proxy.port()));
        ui->proxyuserEdit->setText(proxy.user());
        ui->proxypassEdit->setText(proxy.password());
    }
}

NetworkSettingsDialog::~NetworkSettingsDialog()
{
    delete ui;
}

void NetworkSettingsDialog::accept()
{
    QString host = ui->proxyhostEdit->text().trimmed();

    if (host.isEmpty())
    {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }
    else
    {
        QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpCachingProxy, host,
                                                         ui->proxyportEdit->text().toInt(),
                                                         ui->proxyuserEdit->text(),
                                                         ui->proxypassEdit->text()));
    }

    QDialog::accept();
}
