/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "ui_mainwindow.h"
#include "mainwindow.h"
#include <QFileDialog>
#include "data.h"
#include <QVector>
#include "about.h"


static char AboutStr[] =
"<big>"
"Speedometer <b>plotter</b> GUI version 1.0 " __DATE__
"<br>"
"Copyright 2013 Intel Corporation All Rights Reserved"
"<br><br>"
"<a href=\"http://01.org/lightweight-performance-tools-xeon-and-xeon-phi\">"
"http://01.org/lightweight-performance-tools-xeon-and-xeon-phi</a><br>";

static QDialog *About;
Ui_AboutDialog adialog;

static QString FileName;

std::vector< std::vector<double> > vData;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    fs(0), ms(0)
{

    ui->setupUi(this);
    About = new QDialog(this);
    adialog.setupUi(About);
    adialog.aboutLabel->setText(AboutStr);
    plotZoomer = new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
        ui->plotWidget->canvas());
    connect(plotZoomer, SIGNAL(zoomed(const QRectF &)),
            this, SLOT(on_plotZoom()));

    ui->plotWidget->setAxisScale(QwtPlot::yLeft, 0.0, 110.0);
    ui->plotWidget->setAxisTitle(QwtPlot::yLeft, "%Peak");
    ui->plotWidget->setAxisScale(QwtPlot::xBottom, 0.0, 10.0);
    ui->plotWidget->setAxisTitle(QwtPlot::xBottom, "Seconds");
    ui->plotWidget->setAxisScale(QwtPlot::yRight, 0.0, 240.0);
    ui->plotWidget->setAxisTitle(QwtPlot::yRight, "Threads");
    ui->plotWidget->enableAxis(QwtPlot::yRight);
}

void
MainWindow::on_plotZoom()
{
    // This is supposed to take the rectangle as an argument but I can't
    // get that to work... no matter, it's available from the Zoomer
    QRectF zoomr = plotZoomer->zoomRect();
    double left = zoomr.left();
    double right =  zoomr.right();
    std::vector<double> &vTime = vData[0];
    const int n = vTime.size();
    int l = -2, r = -2;
    for (int i = 0; i < n; ++i)
    {
        if (l == -2 && vTime[i] > left)
            l = i - 1;
        if (r == -2 && vTime[i] > right)
            r = i - 1;
    }
    if (l == -1) l = 0;
    if (l == -2) l = 0;
    if (r == -1) r = 0;
    if (r == -2) r = n-1;
    computeavg(l, r);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    FileName = QFileDialog::getOpenFileName(this,
         tr("Open Plot"), ".", tr("Text Files (*.csv *.txt);;All Files (*.*)"));
    printf("Filename = %s\n", FileName.toUtf8().constData());
    on_plotButton_clicked();
}


void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionAbout_triggered()
{
    About->show();
}

static inline double
roundone(double d)
{
    return round(d*10.0) / 10.0;
}

void
MainWindow::computeavg(int low, int high)
{

    int nl = ms->nlabels();
    double vals[nl];
    memset(vals, 0, sizeof(vals));
    double pcts[nl];
    for (int i = low; i < high; ++i)
    {
        for (int j = 0; j < nl; ++j)
        {
            if (ms->name(j) == "Time" || ms->name(j) == "Threads")
                continue;
            vals[j] += vData[j][i];
        }
    }

    int n = high - low + 1;
    int i = 0;
    for (int j = 0; j < nl; ++j)
    {
        if (ms->name(j) == "Time" || ms->name(j) == "Threads")
            continue;
        vals[j] /= n;
        pcts[j] = (vals[j] / ms->max(j)) * 100.0;
        lcdPct[i]->qlcd()->display(roundone(pcts[j]));
        lcdVal[i]->qlcd()->display(roundone(vals[j]*ms->factor(j)));
        ++i;
    }
}


// This is no longer a button
void MainWindow::on_plotButton_clicked()
{

    if (ms)
    {
        ms->close();
        delete ms;
    }
    if (fs)
    {
        fs->close();
        delete fs;
    }
    fs = new fileStream();
    fs->setFilename(FileName.toUtf8().constData());
    fs->open();
    ms = new metricStream();
    ms->open(fs);

    for (unsigned int i = 0; i < curves.size(); ++i)
    {
        delete curves[i];
    }
    curves.clear();

    std::vector<QColor> colors;
    //     - Color symbols: ColorBrewer.org
    colors.push_back(QColor(0x8D, 0xD3, 0xC7));
    colors.push_back(QColor(0xFF, 0xFF, 0xB3));
    colors.push_back(QColor(0xBE, 0xBA, 0xDA));
    colors.push_back(QColor(0xFB, 0x80, 0x72));
    colors.push_back(QColor(0x80, 0xB1, 0xD3));
    colors.push_back(QColor(0xFD, 0xB4, 0x62));
    colors.push_back(QColor(0xB3, 0xDE, 0x69));
    colors.push_back(QColor(0xFC, 0xCD, 0xE5));
    const int n = ms->nlabels();
    for (int i = 1; i < n; ++i)
    {
        curves.push_back(new QwtPlotCurve());
        curves[i-1]->setTitle(ms->name(i).c_str());
        curves[i-1]->setPen(colors[i], 4);
        curves[i-1]->attach(ui->plotWidget);
        if (ms->name(i) == "Threads")
        {
            curves[i-1]->setYAxis(QwtPlot::yRight);
            ui->plotWidget->setAxisScale(QwtPlot::yRight, 0.0, ms->max(i));
        }
    }
    ui->plotWidget->insertLegend(new QwtLegend());

    for (int i = 0; i < n; ++i)
        vData.push_back(std::vector<double>());
    int nv = 0;
    while (ms->advance() > 0)
    {
        for (int i = 0; i < n; ++i)
            vData[i].push_back(ms->raw(i));
        ++nv;
    }

    std::vector< QVector<double> > vScaled;
    for (int j = 0; j < n; ++j)
        vScaled.push_back(QVector<double>());

    for (int i = 0; i < nv; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            if (ms->name(j) == "Time" || ms->name(j) == "Threads")
                vScaled[j].push_back(vData[j][i]);
            else
                vScaled[j].push_back((vData[j][i] / ms->max(j)) * 100.0);
        }
    }
    QVector<double> &vTime = vScaled[0];
    assert(nv == vTime.size());
    for (int j = 1; j < n; ++j)
        curves[j-1]->setSamples(vTime, vScaled[j]);
    ui->plotWidget->setAxisScale(QwtPlot::xBottom, 0.0, vTime[nv-1]);
    plotZoomer->setZoomBase(true);

    for (unsigned int i = 0; i < lcdVal.size(); ++i)
    {
        delete lcdVal[i];
        delete lcdPct[i];
    }
    lcdVal.clear();
    lcdPct.clear();

    // XXX Probably leaking some labels and layouts here
    for (int j = 0; j < n; ++j)
    {
        if (ms->name(j) == "Time" || ms->name(j) == "Threads")
            continue;
        lcd *lcd1 = new lcd(ms->units(j).c_str());
        lcd *lcd2 = new lcd("% Peak");
        lcdVal.push_back(lcd1);
        lcdPct.push_back(lcd2);
        QLabel *l = new QLabel(ms->name(j).c_str());   // should be longer name?
        QVBoxLayout *vl = new QVBoxLayout();
        vl->addWidget(l);
        vl->addLayout(lcd1->layout());
        vl->addLayout(lcd2->layout());
        ui->mainHoriz->addLayout(vl);
    }

    computeavg(0, nv-1);
}
