/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "speedo_meter.h"
#include <vector>
#include "data.h"
#include "qwt_plot_curve.h"
#include "qwt_legend.h"
#include <QVBoxLayout>
#include <QLCDNumber>
#include <QLabel>

namespace Ui {
class MainWindow;
}

class speedo
{
public:
    SpeedoMeter *sm;
    QLCDNumber *ln;
    QLabel *l;
    QVBoxLayout *vl;
    speedo(const char *name, const char *units, double max, double factor)
    {
        sm = new SpeedoMeter();
        sm->setMinimumSize(QSize(150,150));
        sm->setLineWidth(4);
        sm->setLabel(units);
        sm->setUpperBound(max*factor);
        ln = new QLCDNumber();
        ln->setSegmentStyle(QLCDNumber::Flat);
        l = new QLabel(name);
        vl = new QVBoxLayout();
        vl->addWidget(l);
        vl->addWidget(sm);
        vl->addWidget(ln);
    }
    ~speedo()
    {
        delete l;
        delete sm;
        delete ln;
        delete vl;
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private Q_SLOTS:
    void on_actionOpen_triggered();

    void on_actionQuit_triggered();

    void on_actionAbout_triggered();

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    std::vector<speedo *>numbers;
    std::vector<QwtPlotCurve *>curves;
};

#endif // MAINWINDOW_H
