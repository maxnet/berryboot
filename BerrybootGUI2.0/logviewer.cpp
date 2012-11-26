/* Berryboot -- log viewer dialog
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


#include "logviewer.h"
#include "ui_logviewer.h"
#include <QFileSystemWatcher>
#include <QFile>

LogViewer::LogViewer(QString logfilename, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogViewer),
    _logfilename(logfilename)
{
    ui->setupUi(this);
    _fsw = new QFileSystemWatcher(this);
    connect(_fsw, SIGNAL(fileChanged(QString)), this, SLOT(reloadLog()));
    _fsw->addPath(logfilename);
    reloadLog();
}

LogViewer::~LogViewer()
{
    delete ui;
}

void LogViewer::reloadLog()
{
    QFile f(_logfilename);
    f.open(f.ReadOnly);
    QString txt = f.readAll();
    f.close();
    ui->textedit->setPlainText(txt);
}
