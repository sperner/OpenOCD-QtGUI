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

#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QProcess>
//#include <QtTelnet>
#include "QtTelnet/qttelnet.h"
#include <QtGui/QWidget>
#include <QFile>

#define DIR_FILE_NAME "/tmp/oocdqt-recentdir.dat"

namespace Ui
{
    class MainWidget;
}

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = 0);
    ~MainWidget();

private:
    QString stripCR(const QString &msg);
    void removeEmptyLines();


private slots:
// control buttons:
    void connectToServer();
    void telnetConnected();
    void telnetConnectionError();
    void resetOocd();
    void telnetMessage(const QString &msg);
    void telnetData();
    void ramFileSelect();
    void ramLoad();
    void flashFileSelect();
    void flashLoad();
// command buttons:
    void softReset();
    void reset();
    void halt();
    void resume();
    void poll();
    void eraseFlash();
    void showMemory();
    void remap();
    void peripheralReset();
    void cpuReset();
// openocd tab:
    void ocdConfigFileSelect();
    void ocdConfigStart();
    void openOcdMessage();
    void openOcdStdout();
    void openOcdStderr();
    void editUndo();
    void editRedo();
    void editReload();
    void editSave();
// config tab
    void selectConfigFile();
    void loadConfiguration();
    void saveConfiguration();

private:
    Ui::MainWidget *main;
    QProcess *openOCD;
    QtTelnet *telnet;
    QString recentDir;
};

#endif // MAINWIDGET_H
