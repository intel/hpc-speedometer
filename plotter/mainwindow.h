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
#include <qwt_plot_zoomer.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>
#include <QLCDNumber>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <vector>
#include "data.h"

namespace Ui {
class MainWindow;
class lcd;
}
class lcd;


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

    void on_plotZoom();

private:
    Ui::MainWindow *ui;
    void computeavg(int low, int high);
    void on_plotButton_clicked();
    QwtPlotZoomer *plotZoomer;
    std::vector<QwtPlotCurve *>curves;
    std::vector<lcd *>lcdVal;
    std::vector<lcd *>lcdPct;
    fileStream *fs;
    metricStream *ms;
};

class lcd
{
    QLCDNumber *m_qp;
    QLabel *m_label;
    QHBoxLayout *m_layout;

public:
    lcd(QString name)
    {
        m_layout = new QHBoxLayout();
        m_qp = new QLCDNumber();
        m_qp->setSegmentStyle(QLCDNumber::Flat);
        m_qp->resize(70, 40);
        m_label = new QLabel(name);
        m_layout->addWidget(m_qp);
        m_layout->addWidget(m_label);
    }
    ~lcd()
    {
        delete m_layout;
        delete m_qp;
        delete m_label;
    }
    QLayout *layout() { return m_layout; }
    QLCDNumber *qlcd() { return m_qp; }
};

#endif // MAINWINDOW_H
