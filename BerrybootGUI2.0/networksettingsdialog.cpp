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
#include "installer.h"
#include <QSettings>
#include <QDebug>

NetworkSettingsDialog::NetworkSettingsDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NetworkSettingsDialog),
    _i(i)
{
    ui->setupUi(this);
    QValidator *val = new QIntValidator(this);
    ui->proxyportEdit->setValidator(val);

    QSettings *s = _i->settings();
    s->beginGroup("proxy");
    ui->proxyhostEdit->setText(s->value("hostname").toString());
    ui->proxyportEdit->setText(QString::number(s->value("port", 8080).toInt()));
    ui->proxyuserEdit->setText(s->value("user").toString());
    ui->proxypassEdit->setText(s->value("password").toString());
    s->endGroup();
}

NetworkSettingsDialog::~NetworkSettingsDialog()
{
    delete ui;
}

void NetworkSettingsDialog::accept()
{
    QString host = ui->proxyhostEdit->text().trimmed();
    QSettings *s = _i->settings();
    s->beginGroup("proxy");

    if (host.isEmpty())
    {
        s->remove("");
    }
    else
    {
        s->setValue("type", 4 /*QNetworkProxy::HttpCachingProxy*/);
        s->setValue("hostname", host);
        s->setValue("port", ui->proxyportEdit->text().toInt());
        s->setValue("user", ui->proxyuserEdit->text());
        s->setValue("password", ui->proxypassEdit->text().toAscii().toBase64());
    }

    s->endGroup();
    s->sync();
    QDialog::accept();
}
