/* Berryboot -- configuration edit dialog
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

#include "confeditdialog.h"
#include "ui_confeditdialog.h"
#include <QFile>
#include <QPlainTextEdit>
#include <unistd.h>

/* Private class - For each configuration file there is a tab class */
class ConfEditDialogTab : public QWidget
{
public:
    ConfEditDialogTab(const QString &title, const QString &filename, bool ro, QTabWidget *parent)
        : QWidget(parent), _file(filename), _ro(ro)
    {
        if (_file.open(_file.ReadOnly))
        {
            _origData = _file.readAll();
            _file.close();
        }

        _textEdit = new QPlainTextEdit();
        _textEdit->setPlainText(_origData);
        if (ro)
            _textEdit->setReadOnly(true);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addWidget(_textEdit);
        setLayout(layout);
        parent->addTab(this, title);
    }

    void save()
    {
        QByteArray txt = _textEdit->toPlainText().toLatin1();

        if (!_ro && txt != _origData)
        {
            if (_file.open(_file.WriteOnly))
            {
                _file.write(txt);
                _file.close();
            }
            else
            {
                // TODO: write error handling
            }
        }
    }

protected:
    QFile _file;
    QByteArray _origData;
    bool _ro;
    QPlainTextEdit *_textEdit;
};
/* --- */


ConfEditDialog::ConfEditDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfEditDialog)
{
    ui->setupUi(this);
    ui->tabWidget->clear();
    _tabs.append(new ConfEditDialogTab("cpuinfo", "/proc/cpuinfo", true, ui->tabWidget));
    _tabs.append(new ConfEditDialogTab("config.txt", "/boot/config.txt", false, ui->tabWidget));
    _tabs.append(new ConfEditDialogTab("cmdline.txt", "/boot/cmdline.txt", false, ui->tabWidget));
    _tabs.append(new ConfEditDialogTab("wpa_supplicant.conf", "/boot/wpa_supplicant.conf", false, ui->tabWidget));
    _tabs.append(new ConfEditDialogTab("iscsi.sh", "/boot/iscsi.txt", false, ui->tabWidget));
    _tabs.append(new ConfEditDialogTab("uEnv.txt", "/boot/uEnv.txt", false, ui->tabWidget));
}

ConfEditDialog::~ConfEditDialog()
{
    delete ui;
}

void ConfEditDialog::accept()
{
    foreach (ConfEditDialogTab *tab, _tabs)
    {
        tab->save();
    }
    sync();
    QDialog::accept();
}
