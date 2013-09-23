/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef DIALOG_H
#define DIALOG_H
#include <QLabel>
#include <QDialog>
#include <QDialogButtonBox>
#include <QWidget>
#include <QLineEdit>
#include <QGridLayout>

class dialog
{
public:
    QDialogButtonBox *buttonBox;
    QWidget *widget;
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *hostName;
    QLabel *label_2;
    QLineEdit *port;

    void init(QDialog *Connection)
    {

        if (Connection->objectName().isEmpty())
            Connection->setObjectName(QString::fromUtf8("Connection"));
        Connection->setWindowModality(Qt::ApplicationModal);
        Connection->resize(377, 163);
        buttonBox = new QDialogButtonBox(Connection);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(10, 110, 341, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        widget = new QWidget(Connection);
        widget->setObjectName(QString::fromUtf8("widget"));
        widget->setGeometry(QRect(20, 20, 335, 71));
        gridLayout = new QGridLayout(widget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(QString("Hostname"), widget);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        hostName = new QLineEdit(widget);
        hostName->setObjectName(QString::fromUtf8("hostName"));

        gridLayout->addWidget(hostName, 0, 1, 1, 1);

        label_2 = new QLabel(QString("Port"), widget);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        port = new QLineEdit(widget);
        port->setObjectName(QString::fromUtf8("port"));

        gridLayout->addWidget(port, 1, 1, 1, 1);


        QObject::connect(buttonBox, SIGNAL(accepted()), Connection, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), Connection, SLOT(reject()));

        QMetaObject::connectSlotsByName(Connection);
    }
};
#endif
