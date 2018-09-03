/* Berryboot -- iSCSI settings dialog
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

#include "iscsidialog.h"
#include "ui_iscsidialog.h"
#include "networksettingsdialog.h"
#include <QProgressDialog>
#include <QApplication>
#include <QFile>
#include <QProcess>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

iSCSIDialog::iSCSIDialog(Installer *i, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::iSCSIDialog),
    _i(i)
{
    ui->setupUi(this);
    ui->targetEdit->setFocus();

    QPushButton *button = new QPushButton(QIcon(":/icons/server.png"), tr("Network settings"), this);
    connect(button, SIGNAL(clicked()), this, SLOT(onNetworkSettings()));
    ui->buttonBox->addButton(button, QDialogButtonBox::ActionRole);
}

iSCSIDialog::~iSCSIDialog()
{
    delete ui;
}

void iSCSIDialog::accept()
{
    if ( ui->initiatorEdit->text().isEmpty() || ui->targetEdit->text().isEmpty() || ui->serverEdit->text().isEmpty() )
    {
        QMessageBox::critical(this, tr("Error"), tr("Fill in all required fields"), QMessageBox::Close);
        return;
    }

    QProgressDialog qpd(tr("Loading iSCSI drivers..."), QString(),0,0, this);
    qpd.show();
    QApplication::processEvents();

    if (!QFile::exists("/sys/module/iscsi_tcp"))
    {
        _i->prepareDrivers();
        QProcess::execute("/sbin/modprobe iscsi_tcp");
    }

    qpd.setLabelText(tr("Connecting to iSCSI server..."));
    QProcess proc;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&proc, SIGNAL(finished(int)), &qpd, SLOT(close()));
    connect(&timer, SIGNAL(timeout()), &proc, SLOT(kill()));

    // TODO: properly escape characters
    QString cmd = "/sbin/iscsistart -i \""+ui->initiatorEdit->text()+"\""+" -g 1 -t \""+ui->targetEdit->text()+"\" -a \""+ui->serverEdit->text()+"\"";
    if ( !ui->passEdit->text().isEmpty() )
        cmd += " -u \""+ui->userEdit->text()+"\" -w \""+ui->passEdit->text()+"\"";

    proc.start(cmd);
    timer.start(5000);

    while (proc.state() != proc.NotRunning)
        qpd.exec();

    if (proc.exitCode() != 0 || !timer.isActive())
    {
        timer.stop();
        QMessageBox::critical(this, tr("Error"), tr("Error connecting to target. Check settings"), QMessageBox::Close);
        return;
    }
    timer.stop();

    QFile f("/boot/iscsi.sh");
    f.open(f.WriteOnly);
    f.write(cmd.toLatin1());
    f.close();

    QDialog::accept();
}

void iSCSIDialog::onNetworkSettings()
{
    NetworkSettingsDialog ns(_i, this);
    ns.exec();
}
