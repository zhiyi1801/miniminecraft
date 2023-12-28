#include "mainwindow.h"
#include <iostream>
#include <ostream>
#include <ui_mainwindow.h>
#include "cameracontrolshelp.h"
#include <QFileDialog>
#include <QResizeEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), cHelp()
{
    ui->setupUi(this);
    ui->mygl->setFocus();
    this->playerInfoWindow.show();
    playerInfoWindow.move(QGuiApplication::primaryScreen()->availableGeometry().center() - this->rect().center() + QPoint(this->width() * 0.75, 0));

    connect(ui->mygl, SIGNAL(sig_sendPlayerPos(QString)), &playerInfoWindow, SLOT(slot_setPosText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerVel(QString)), &playerInfoWindow, SLOT(slot_setVelText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerAcc(QString)), &playerInfoWindow, SLOT(slot_setAccText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerLook(QString)), &playerInfoWindow, SLOT(slot_setLookText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerChunk(QString)), &playerInfoWindow, SLOT(slot_setChunkText(QString)));
    connect(ui->mygl, SIGNAL(sig_sendPlayerTerrainZone(QString)), &playerInfoWindow, SLOT(slot_setZoneText(QString)));
    connect(ui->mygl, SIGNAL(sig_sentPlayerFps(QString)), &playerInfoWindow, SLOT(slot_setFpsText(QString)));

    connect(ui->actionOpen_Texture_Files, &QAction::triggered, this, &MainWindow::slot_openTextureDialog);
    connect(ui->actionLoad_Test_Lava, &QAction::triggered, ui->mygl, &MyGL::slot_testMakeLava);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::slot_openTextureDialog()
{
    std::cout << "openTextureDialog reached" << std::endl;
    QFileDialog dialog(this);
    connect(&dialog, &QFileDialog::fileSelected, ui->mygl, &MyGL::slot_testMakeLava);
    dialog.setDirectory("../textures");
    QString file;
    if (dialog.exec()) {
    }
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::exit();
}

void MainWindow::on_actionCamera_Controls_triggered()
{
    cHelp.show();
}
