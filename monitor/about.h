#ifndef ABOUT_H
#define ABOUT_H
#include <QDialog>
#include <QLabel>
#include <QDialogButtonBox>

class Ui_AboutDialog
{
public:
    QDialogButtonBox *aboutOK;
    QLabel *aboutLabel;

    void setupUi(QDialog *AboutDialog)
    {
        if (AboutDialog->objectName().isEmpty())
            AboutDialog->setObjectName(QStringLiteral("AboutDialog"));
        AboutDialog->resize(400, 300);
        AboutDialog->setWindowTitle("About monitor");
        aboutOK = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal,AboutDialog);
        aboutOK->setObjectName(QStringLiteral("aboutOK"));
        aboutOK->setGeometry(QRect(170, 250, 60, 20));
        aboutLabel = new QLabel(AboutDialog);
        aboutLabel->setObjectName(QStringLiteral("aboutLabel"));
        aboutLabel->setGeometry(QRect(10, 10, 380, 150));
        QObject::connect(aboutOK, SIGNAL(accepted()), AboutDialog, SLOT(accept()));
        QMetaObject::connectSlotsByName(AboutDialog);

    } // setupUi

};

#endif
