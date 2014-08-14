/* Berryboot -- OS image properties dialog
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


#include "editdialog.h"
#include "ui_editdialog.h"
#include <QRegExp>

EditDialog::EditDialog(const QString &filename, bool usingMemsplits, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditDialog)
{
    ui->setupUi(this);

    QString f = filename;
    int pos = f.lastIndexOf(".img");
    if (pos != -1)
    {
        if (f.endsWith("128"))
            ui->memsplitCombo->setCurrentIndex(1);
        else if (f.endsWith("192"))
            ui->memsplitCombo->setCurrentIndex(2);
        else if (f.endsWith("224"))
            ui->memsplitCombo->setCurrentIndex(3);
        else if (f.endsWith("240"))
            ui->memsplitCombo->setCurrentIndex(4);
        else if (f.endsWith("256"))
            ui->memsplitCombo->setCurrentIndex(5);

        f = f.mid(0, pos);
    }
    f.replace('_', ' ');

    ui->nameEdit->setText(f);
    //QValidator *validator = new QRegExpValidator(QRegExp("^[^\\_\\/\\\\]+$"), this);
    QValidator *validator = new QRegExpValidator(QRegExp("^[a-zA-Z0-9 \\.\\+\\-]+$"), this);
    ui->nameEdit->setValidator(validator);

    if (usingMemsplits)
        ui->cmaLabel->setHidden(true);
}

EditDialog::~EditDialog()
{
    delete ui;
}

QString EditDialog::filename() const
{
    QString f = ui->nameEdit->text();
    f.replace(' ', '_');

    switch (ui->memsplitCombo->currentIndex())
    {
        case 1:
            f += ".img128";
            break;

        case 2:
            f += ".img192";
            break;

        case 3:
            f += ".img224";
            break;

        case 4:
            f += ".img240";
            break;

        case 5:
            f += ".img256";
            break;

        default: /* No preference */
            f += ".img";
    }

    return f;
}
