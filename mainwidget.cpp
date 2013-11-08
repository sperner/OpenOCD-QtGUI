/*
Graphical frontend for the Open On-Chip Debugger
Copyright (C) 2013 Sven Sperner

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define SAM7_VERSION "0.3.2"

#include "mainwidget.h"
#include "ui_mainwidget.h"
#include <QStringList>
#include <QScrollBar>
#include <QFileDialog>
#include <QByteArray>
#include <QRect>

#include <iostream>
using namespace std;


MainWidget::MainWidget(QWidget *parent) : QWidget(parent), main(new Ui::MainWidget)
{
    main->setupUi(this);

    setGeometry(QRect(100,100,800,480));
    setWindowTitle(QString("SAM7 openOCD GUI v") + SAM7_VERSION);

    openOCD = new QProcess(this);
    telnet = new QtTelnet(this);

// control buttons
    connect(main->pushButtonOocdConnect, SIGNAL(clicked()), this, SLOT(connectToServer()));
    connect(telnet, SIGNAL(message(QString)), this, SLOT(telnetMessage(QString)));
    connect(telnet->socket(), SIGNAL(connected()), this, SLOT(telnetConnected()));
    connect(telnet, SIGNAL(connectionError(QAbstractSocket::SocketError)), this, SLOT(telnetConnectionError()));

    connect(main->pushButtonOocdReset, SIGNAL(clicked()), this, SLOT(resetOocd()));
    connect(main->lineEditInput, SIGNAL(returnPressed()), this, SLOT(telnetData()));

// ram
    connect(main->pushButtonRamFile, SIGNAL(clicked()), this, SLOT(ramFileSelect()));
    connect(main->pushButtonRamLoad, SIGNAL(clicked()), this, SLOT(ramLoad()));

// flash
    connect(main->pushButtonFlashFile, SIGNAL(clicked()), this, SLOT(flashFileSelect()));
    connect(main->pushButtonFlashLoad, SIGNAL(clicked()), this, SLOT(flashLoad()));

// command buttons
    connect(main->pushButtonSoftReset, SIGNAL(clicked()), this, SLOT(softReset()));
    connect(main->pushButtonReset, SIGNAL(clicked()), this, SLOT(reset()));
    connect(main->pushButtonResume, SIGNAL(clicked()), this, SLOT(resume()));
    connect(main->pushButtonHalt, SIGNAL(clicked()), this, SLOT(halt()));
    connect(main->pushButtonPoll, SIGNAL(clicked()), this, SLOT(poll()));
    connect(main->pushButtonShowMem, SIGNAL(clicked()), this, SLOT(showMemory()));
    connect(main->pushButtonEraseFlash, SIGNAL(clicked()), this, SLOT(eraseFlash()));
    connect(main->pushButtonRemap, SIGNAL(clicked()), this, SLOT(remap()));
    connect(main->pushButtonPeriphReset, SIGNAL(clicked()), this, SLOT(peripheralReset()));
    connect(main->pushButtonCpuReset, SIGNAL(clicked()), this, SLOT(cpuReset()));

// openocd tab
    connect(main->pushButtonOcdConfigFile, SIGNAL(clicked()), this, SLOT(ocdConfigFileSelect()));
    connect(main->pushButtonOcdConfigStart, SIGNAL(clicked()), this, SLOT(ocdConfigStart()));

    connect(openOCD, SIGNAL(readyRead()), this, SLOT(openOcdMessage()));
    connect(openOCD, SIGNAL(readyReadStandardOutput()), this, SLOT(openOcdStdout()));
    connect(openOCD, SIGNAL(readyReadStandardError()), this, SLOT(openOcdStderr()));

    connect(main->pushButtonUndo, SIGNAL(clicked()), this, SLOT(editUndo()));
    connect(main->pushButtonRedo, SIGNAL(clicked()), this, SLOT(editRedo()));
    connect(main->pushButtonReload, SIGNAL(clicked()), this, SLOT(editReload()));
    connect(main->pushButtonSave, SIGNAL(clicked()), this, SLOT(editSave()));

// configuration tab
    connect(main->pushButtonGuiConfigFile, SIGNAL(clicked()), this, SLOT(selectConfigFile()));
    connect(main->pushButtonGuiConfigLoad, SIGNAL(clicked()), this, SLOT(loadConfiguration()));
    connect(main->pushButtonGuiConfigSave, SIGNAL(clicked()), this, SLOT(saveConfiguration()));

    QFile dirFile(DIR_FILE_NAME);
    if (dirFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream dirin(&dirFile);
        recentDir = dirin.readLine(256);
    }
    
    QFile cfgFile(main->lineEditOcdConfig->text());
    if (cfgFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream cfgin(&cfgFile);
        main->textEditOcdConfig->setText(cfgin.readAll());
    }
}

MainWidget::~MainWidget()
{
    QFile dirFile(DIR_FILE_NAME);
    if (dirFile.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream out(&dirFile);
        out << recentDir;
    }

    delete main;
}



// private Slots:

// control buttons:
void MainWidget::connectToServer()
{
    if (main->pushButtonOocdConnect->text() == "Connect")
        telnet->connectToHost(main->lineEditHost->text(), main->lineEditPort->text().toInt());
    else
    {
        telnet->close();
        main->textEditOutput->append("GUI: Connection closed");
        main->pushButtonOocdConnect->setText("Connect");
    }
}

void MainWidget::telnetConnected()
{
    main->textEditOutput->append("GUI: Connected to " + main->lineEditHost->text() + ":" + main->lineEditPort->text());
    main->pushButtonOocdConnect->setText("Disconnect");
}

void MainWidget::telnetConnectionError()
{
    main->textEditOutput->append("GUI: Can not connect to " + main->lineEditHost->text() + ":" + main->lineEditPort->text());
}

void MainWidget::resetOocd()
{
    if (main->pushButtonOocdConnect->text() == "Disconnect")
    {
        telnet->close();
        telnet->connectToHost(main->lineEditHost->text(), main->lineEditPort->text().toInt());
        main->textEditOutput->append("GUI: Reset Connection");
    }
    else
        main->textEditOutput->append("GUI: Not connected");
}


void MainWidget::telnetMessage(const QString &msg) // receive output
{
    main->textEditOutput->append(stripCR(msg));
    QScrollBar *s = main->textEditOutput->verticalScrollBar();
    s->setValue(s->maximum());
}

void MainWidget::telnetData() // send command
{
    telnet->sendData(main->lineEditInput->text());
    main->lineEditInput->clear();
}


void MainWidget::ramFileSelect()
{
    QFileDialog fDlg(this, "Select RAM Image", recentDir, "*.bin *.BIN *.elf *.ELF");

    if(fDlg.exec())
    {
        main->lineEditRam->setText(fDlg.selectedFiles().at(0));
        recentDir = fDlg.directory().absolutePath();
    }
}

void MainWidget::ramLoad() // download image to RAM
{
    QString buffer = main->lineEditRam->text();
    int tmp = buffer.size();
    
    if ((buffer[tmp-3] == 'e' && buffer[tmp-2] == 'l' && buffer[tmp-1] == 'f') ||
        (buffer[tmp-3] == 'E' && buffer[tmp-2] == 'L' && buffer[tmp-1] == 'F'))
    {
        telnet->sendData("soft_reset_halt");
        telnet->sendData("load_image " + main->lineEditRam->text() + " 0x0 elf");
    }
    else if ((buffer[tmp-3] == 'b' && buffer[tmp-2] == 'i' && buffer[tmp-1] == 'n') ||
	     (buffer[tmp-3] == 'B' && buffer[tmp-2] == 'I' && buffer[tmp-1] == 'N'))
    {
        telnet->sendData("soft_reset_halt");
        telnet->sendData("load_image " + main->lineEditRam->text() + " 0x200000 bin");
    }
}

void MainWidget::flashFileSelect()
{
    QFileDialog fDlg(this, "Select FLASH Image", recentDir, "*.bin *.BIN *.elf *.ELF");

    if(fDlg.exec())
    {
        main->lineEditFlash->setText(fDlg.selectedFiles().at(0));
        recentDir = fDlg.directory().absolutePath();
    }
}

void MainWidget::flashLoad() // download image to FLASH
{
    QString buffer = main->lineEditFlash->text();
    int tmp = buffer.size();
    
    if ((buffer[tmp-3] == 'e' && buffer[tmp-2] == 'l' && buffer[tmp-1] == 'f') ||
        (buffer[tmp-3] == 'E' && buffer[tmp-2] == 'L' && buffer[tmp-1] == 'F'))
    {
        telnet->sendData("soft_reset_halt");
        if (main->checkBoxErase->isChecked())
            telnet->sendData(main->lineEditFlashWriteCmd->text() + " erase " + main->lineEditFlash->text() + " 0x0 elf");
        else
            telnet->sendData(main->lineEditFlashWriteCmd->text() + " " + main->lineEditFlash->text() + " 0x0 elf");
    }
    else if ((buffer[tmp-3] == 'b' && buffer[tmp-2] == 'i' && buffer[tmp-1] == 'n') ||
	     (buffer[tmp-3] == 'B' && buffer[tmp-2] == 'I' && buffer[tmp-1] == 'N'))
    {
        telnet->sendData("soft_reset_halt");
        if (main->checkBoxErase->isChecked())
            telnet->sendData(main->lineEditFlashWriteCmd->text() + " erase " + main->lineEditFlash->text() + " 0x100000 bin");
        else
            telnet->sendData(main->lineEditFlashWriteCmd->text() + " " + main->lineEditFlash->text() + " 0x100000 bin");
    }
}



// command buttons:
void MainWidget::softReset()
{
    telnet->sendData(main->lineEditSoftResetCmd->text());
}

void MainWidget::reset()
{
    telnet->sendData(main->lineEditResetCmd->text());
}

void MainWidget::halt()
{
    telnet->sendData(main->lineEditHaltCmd->text());
}

void MainWidget::resume()
{
    telnet->sendData(main->lineEditResumeCmd->text());
}

void MainWidget::poll()
{
    telnet->sendData(main->lineEditPollCmd->text());
}

void MainWidget::eraseFlash()
{
    telnet->sendData(main->lineEditSoftResetCmd->text());
    telnet->sendData(main->lineEditFlashEraseCmd->text());
}

//
void MainWidget::showMemory()
{
    telnet->sendData("mdw " + main->lineEditBaseAddress->text() + " 0x08");	// base (mapped)
    telnet->sendData("mdw " + main->lineEditFlashAddress->text() + " 0x08");	// flash
    telnet->sendData("mdw " + main->lineEditRamAddress->text() + " 0x08");	// sram
}

void MainWidget::remap()
{
    telnet->sendData("mww " + main->lineEditRemapAddress->text() + " " + main->lineEditRemapValue->text());
}

void MainWidget::peripheralReset()
{
    telnet->sendData("mww " + main->lineEditPeriphResetAddress->text() + " " + main->lineEditPeriphResetValue->text());
}

void MainWidget::cpuReset()
{
    telnet->sendData("mww " + main->lineEditCpuResetAddress->text() + " " + main->lineEditCpuResetValue->text());
}



// openocd tab
void MainWidget::ocdConfigFileSelect()
{
    QFileDialog fDlg(this, "Select OpenOCD Configuration", recentDir, "*.cfg *.conf *.config *.oocd *.open *.openocd");

    if(fDlg.exec())
    {
        main->lineEditOcdConfig->setText(fDlg.selectedFiles().at(0));
        recentDir = fDlg.directory().absolutePath();
    }
    editReload();
}

void MainWidget::ocdConfigStart() // start OpenOCD with Config
{
    if (main->pushButtonOcdConfigStart->text() == "Start")
    {
        QString path = "/usr/bin/openocd";
        QStringList arguments;
        arguments << "-f" << main->lineEditOcdConfig->text();
        openOCD->start(path, arguments);
        main->textEditOcdTerminal->append("GUI: OpenOCD started");
        main->pushButtonOcdConfigStart->setText("Stop");
    }
    else
    {
        openOCD->terminate();
        if (!openOCD->waitForFinished(1000))
            openOCD->kill();
        main->textEditOcdTerminal->append("GUI: OpenOCD stopped");
        main->pushButtonOcdConfigStart->setText("Start");
    }
}

void MainWidget::openOcdMessage()
{
    main->textEditOcdTerminal->append(stripCR(openOCD->readAll()));
    QScrollBar *s = main->textEditOcdTerminal->verticalScrollBar();
    s->setValue(s->maximum());
}

void MainWidget::openOcdStdout()
{
    main->textEditOcdTerminal->append(stripCR(openOCD->readAllStandardOutput()));
    QScrollBar *s = main->textEditOcdTerminal->verticalScrollBar();
    s->setValue(s->maximum());
}

void MainWidget::openOcdStderr()
{
    main->textEditOcdTerminal->append(stripCR(openOCD->readAllStandardError()));
    QScrollBar *s = main->textEditOcdTerminal->verticalScrollBar();
    s->setValue(s->maximum());
}

void MainWidget::editUndo()
{
    main->textEditOcdConfig->undo();
}

void MainWidget::editRedo()
{
    main->textEditOcdConfig->redo();
}

void MainWidget::editReload()
{
    QFile cfgFile(main->lineEditOcdConfig->text());
    if (cfgFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream cfgin(&cfgFile);
        main->textEditOcdConfig->setText(cfgin.readAll());
        main->textEditOcdTerminal->append("GUI: OpenOCD-Config loaded");
    }
}

void MainWidget::editSave()
{
    QFile cfgFile(main->lineEditOcdConfig->text());
    if (cfgFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream cfgout(&cfgFile);
        cfgout << main->textEditOcdConfig->toPlainText();
        main->textEditOcdTerminal->append("GUI: OpenOCD-Config saved");
    }
}



// configuration tab
void MainWidget::selectConfigFile()
{
    QFileDialog fDlg(this, "Select GUI Configuration", recentDir, "*.cfg *.conf *.config");

    if(fDlg.exec())
    {
        main->lineEditGuiConfig->setText(fDlg.selectedFiles().at(0));
        recentDir = fDlg.directory().absolutePath();
    }
}

void MainWidget::loadConfiguration()
{
    QString buffer;
    QStringList buflist;
    QFile cfgFile(main->lineEditGuiConfig->text());
    if (cfgFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream cfgIn(&cfgFile);
        while (!cfgIn.atEnd())
        {
            buffer = cfgIn.readLine();
            if (buffer[0] == '#')	continue;	//skip comments
            buflist = buffer.split(' ');
            if (buflist[0] == "BASE") {
                main->lineEditBaseAddress->setText(buflist[2]);
            }
            else if (buflist[0] == "FLASH") {
                main->lineEditFlashAddress->setText(buflist[2]);
            }
            else if (buflist[0] == "RAM") {
                main->lineEditRamAddress->setText(buflist[2]);
            }
            else if (buflist[0] == "REMAP") {
                main->lineEditRemapAddress->setText(buflist[2]);
                main->lineEditRemapValue->setText(buflist[3]);
            }
            else if (buflist[0] == "RESETCPU") {
                main->lineEditCpuResetAddress->setText(buflist[2]);
                main->lineEditCpuResetValue->setText(buflist[3]);
            }
            else if (buflist[0] == "RESETPERIPH") {
                main->lineEditPeriphResetAddress->setText(buflist[2]);
                main->lineEditPeriphResetValue->setText(buflist[3]);
            }
            else if (buflist[0] == "FLASHPROBE") {
                main->lineEditFlashProbeCmd->setText(buflist[2]+" "+buflist[3]+" "+buflist[4]);
            }
            else if (buflist[0] == "FLASHINFO") {
                main->lineEditFlashInfoCmd->setText(buflist[2]+" "+buflist[3]+" "+buflist[4]);
            }
            else if (buflist[0] == "FLASHERASE") {
                main->lineEditFlashEraseCmd->setText(buflist[2]+" "+buflist[3]+" "+buflist[4]+" "+buflist[5]);
            }
            else if (buflist[0] == "FLASHUNLOCK") {
                main->lineEditFlashUnlockCmd->setText(buflist[2]+" "+buflist[3]+" "+buflist[4]+" "+buflist[5]+" "+buflist[6]);
            }
            else if (buflist[0] == "FLASHWRITE") {
                main->lineEditFlashWriteCmd->setText(buflist[2]+" "+buflist[3]);
            }
            else if (buflist[0] == "ERASESUFFIX") {
                main->lineEditEraseSuffix->setText(buflist[2]);
            }
            else if (buflist[0] == "RAMWRITE") {
                main->lineEditRamWriteCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "RESET") {
                main->lineEditResetCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "HALT") {
                main->lineEditHaltCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "RESUME") {
                main->lineEditResumeCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "POLL") {
                main->lineEditPollCmd->setText(buflist[2]);
            }
            else if (buflist[0] == "SOFTRESET") {
                main->lineEditSoftResetCmd->setText(buflist[2]);
            }
        }
        main->textEditOcdTerminal->append("GUI: GUI-Config loaded");
    }
}

void MainWidget::saveConfiguration()
{
    QFile cfgFile(main->lineEditGuiConfig->text());
    if (cfgFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream cfgOut(&cfgFile);
        cfgOut << "BASE = " << main->lineEditBaseAddress->text() << " " << endl;
        cfgOut << "FLASH = " << main->lineEditFlashAddress->text() << " " << endl;
        cfgOut << "RAM = " << main->lineEditRamAddress->text() << " " << endl;
        cfgOut << "REMAP = " << main->lineEditRemapAddress->text() << " "
                     << main->lineEditRemapValue->text() << endl;
        cfgOut << "RESETCPU = " << main->lineEditCpuResetAddress->text() << " "
                    << main->lineEditCpuResetValue->text() << endl;
        cfgOut << "RESETPERIPH = " << main->lineEditPeriphResetAddress->text() << " "
                       << main->lineEditPeriphResetValue->text() << endl;
        cfgOut << "FLASHPROBE = " << main->lineEditFlashProbeCmd->text() << " " << endl;
        cfgOut << "FLASHINFO = " << main->lineEditFlashInfoCmd->text() << " " << endl;
        cfgOut << "FLASHERASE = " << main->lineEditFlashEraseCmd->text() << " " << endl;
        cfgOut << "FLASHUNLOCK = " << main->lineEditFlashUnlockCmd->text() << " " << endl;
        cfgOut << "FLASHWRITE = " << main->lineEditFlashWriteCmd->text() << " " << endl;
        cfgOut << "ERASESUFFIX = " << main->lineEditEraseSuffix->text() << " " << endl;
        cfgOut << "RAMWRITE = " << main->lineEditRamWriteCmd->text() << " " << endl;
        cfgOut << "RESET = " << main->lineEditResetCmd->text() << " " << endl;
        cfgOut << "HALT = " << main->lineEditHaltCmd->text() << " " << endl;
        cfgOut << "RESUME = " << main->lineEditResumeCmd->text() << " " << endl;
        cfgOut << "POLL = " << main->lineEditPollCmd->text() << " " << endl;
        cfgOut << "SOFTRESET = " << main->lineEditSoftResetCmd->text() << " " << endl;
        main->textEditOcdTerminal->append("GUI: GUI-Config saved as " + cfgFile.fileName());
    }
}



// private Funktions:
QString MainWidget::stripCR(const QString &msg)
{
    QString nmsg(msg);
    nmsg.remove('\r');
    //nmsg.remove('\n\r\n\r');
    nmsg.remove(QRegExp("\033\\[[0-9;]*[A-Za-z]")); // Also remove terminal control codes
    return nmsg.isEmpty() ? NULL : nmsg;
}

void MainWidget::removeEmptyLines()
{

}



