/*
Copyright (c) 2009-2013, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>

#include "dialog.h"
#include "about.h"

static char AboutStr[] = 
"<big>"
"Speedometer <b>monitor</b> GUI version 1.0 " __DATE__
"<br>"
"Copyright 2013 Intel Corporation All Rights Reserved"
"<br><br>"
"<a href=\"http://01.org/lightweight-performance-tools-xeon-and-xeon-phi\">"
"http://01.org/lightweight-performance-tools-xeon-and-xeon-phi</a><br>";

static QString micHostname = "mic0";
static QString micPort = "12345";
static QDialog *Connection;
dialog cdialog;
static QDialog *About;
Ui_AboutDialog adialog;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    Connection = new QDialog(this);
    cdialog.init(Connection);
    About = new QDialog(this);
    adialog.setupUi(About);
    adialog.aboutLabel->setText(AboutStr);
    ui->plotWidget->setAxisScale(QwtPlot::yLeft, 0.0, 110.0);
    ui->plotWidget->setAxisTitle(QwtPlot::yLeft, "%Peak");
    ui->plotWidget->setAxisScale(QwtPlot::xBottom, 0.0, 30.0);
    ui->plotWidget->setAxisTitle(QwtPlot::xBottom, "Seconds");
    ui->plotWidget->setAxisScale(QwtPlot::yRight, 0.0, 240.0);
    ui->plotWidget->setAxisTitle(QwtPlot::yRight, "Threads");
    ui->plotWidget->enableAxis(QwtPlot::yRight);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    int ret;
    ret = Connection->exec();
    if (ret == QDialog::Accepted)
    {
        micHostname = cdialog.hostName->text();
        micPort = cdialog.port->text();
    }
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

void MainWindow::on_pushButton_clicked()
{
    static sockStream *sock;
    static metricStream ms;
    bool paused;

    do
    {
        paused = (ui->pushButton->text() == QString("resume"));
        if (!paused)
            break;

        assert(sock == 0);
        sock = new sockStream();
        ui->pushButton->setText("pause");
        sock->setHostname(micHostname.toLatin1());
        sock->setPort(micPort.toLatin1());
        if (sock->open() != 0)
        {
            printf("Error opening %s:%s\n",
                micHostname.toLatin1().constData(),
                micPort.toLatin1().constData());
            break;
        }

        ms.open(sock);

        ui->plotWidget->setAxisScale(QwtPlot::xBottom, 0.0, 30.0);
        for (unsigned int i = 0; i < curves.size(); ++i)
            delete curves[i];
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
        const int n = ms.nlabels();
        for (int i = 1; i < n; ++i)
        {
            curves.push_back(new QwtPlotCurve());
            curves[i-1]->setTitle(ms.name(i).c_str());
            curves[i-1]->setPen(colors[i], 4);
            curves[i-1]->attach(ui->plotWidget);
            if (ms.name(i) == "Threads")
            {
                curves[i-1]->setYAxis(QwtPlot::yRight);
                ui->plotWidget->setAxisScale(QwtPlot::yRight, 0.0, ms.max(i));
            }
        }
        ui->plotWidget->insertLegend(new QwtLegend());

        for (unsigned int i = 0; i < numbers.size(); ++i)
        {
            delete numbers[i];
        }
        numbers.clear();

        for (int i = 1; i < n; ++i)
        {
            if (ms.name(i) == "Threads")
                continue;
            speedo *s = new speedo(ms.name(i).c_str(), ms.units(i).c_str(),
                ms.max(i), ms.factor(i));
            numbers.push_back(s);
            ui->horizontalLayout->addLayout(s->vl);
        }
        std::vector< QVector<double> > vData;

        for (int i = 0; i < n; ++i)
            vData.push_back(QVector<double>());

        const int nelems = 150;
        double firsttime = 0;
        double maxes[n];
        memset(maxes, 0, sizeof(maxes));
        while (ms.advance() > 0)
        {
            int j = 0;
            for (int i = 0; i < n; ++i)
            {
                if (vData[0].size() >= nelems)
                    vData[i].pop_front();
                if (ms.name(i) != "Threads" && ms.name(i) != "Time")
                {
                    double d = ms.raw(i) * ms.factor(i);
                    numbers[j]->sm->setValue(d);
                    if (d > maxes[i])
                    {
                        maxes[i] = d;
                        numbers[j]->ln->display(maxes[i]);
                    }
                    vData[i].push_back(ms.scaled(i));
                    ++j;
                }
                else if (ms.name(i) == "Time")
                {
                    if (firsttime == 0)
                        firsttime = ms.raw(i);
                    vData[i].push_back(ms.raw(i) - firsttime);
                }
                else
                {
                    vData[i].push_back(ms.raw(i));
                }
            }
            if (vData[0].size() >= nelems)
                ui->plotWidget->setAxisScale(QwtPlot::xBottom,
                    vData[0][0],
                    vData[0][nelems-1]);
            for (int i = 1; i < n; ++i)
                curves[i-1]->setSamples(vData[0], vData[i]);
            ui->plotWidget->replot();
            QApplication::processEvents();
            paused = (ui->pushButton->text() == QString("resume"));
            if (paused)
                break;
        }
    } while (0);

    ui->pushButton->setText("resume");
    ms.close();
    delete sock;
    sock = 0;
}
