/* Berryboot -- login dialog
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


#include "berrybootsettingsdialog.h"
#include "ui_berrybootsettingsdialog.h"
#include "installer.h"
#include <QSettings>
#include <QMessageBox>
#include <QCryptographicHash>

BerrybootSettingsDialog::BerrybootSettingsDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BerrybootSettingsDialog),
    _i(i)
{
    ui->setupUi(this);

    if (_i->hasSettings() && _i->settings()->contains("berryboot/passwordhash") )
    {
        ui->passGroup->setChecked(true);
    }
}

BerrybootSettingsDialog::~BerrybootSettingsDialog()
{
    delete ui;
}

void BerrybootSettingsDialog::accept()
{
    QSettings *s = _i->settings();

    if (ui->passGroup->isChecked())
    {
        QString p = ui->pass1Edit->text();

        if (p.isEmpty())
        {
            QMessageBox::critical(this, tr("Error"), tr("Enter password or uncheck checkbox!"), QMessageBox::Close);
            ui->pass1Edit->setFocus();
            return;
        }

        if (p != ui->pass2Edit->text())
        {
            QMessageBox::critical(this, tr("Error"), tr("Passwords do not match!"), QMessageBox::Close);
            ui->pass1Edit->setFocus();
            return;
        }

        s->setValue("berryboot/passwordhash", QCryptographicHash::hash(p.toUtf8(), QCryptographicHash::Sha1).toHex());
    }
    else
    {
        s->remove("berryboot/passwordhash");
    }

    s->sync();
    QDialog::accept();
}
