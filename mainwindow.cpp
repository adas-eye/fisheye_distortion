#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QImage>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_correction = fisheye_distortion_correction::getInstance();
    QString filepath_default = "C:/Work/Cygwin/home/zixiangz/qt/fisheye_distortion/build/eye_fish.jpg";
    ui->edit_file_location->setText(filepath_default);
    m_correction->setFileLocation(filepath_default);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFileOnClicked() {
    QString filepath = QFileDialog::getOpenFileName(this, tr("open log"), NULL, tr("log file (*)"));
    if (filepath.isEmpty()) {
       qDebug() << "file is empty()" << endl;
       return;
    }
    if (m_correction != NULL) {
        qDebug() << "filepath = " << filepath;
        ui->edit_file_location->setText(filepath);
        m_correction->setFileLocation(filepath);
    }
}

void MainWindow::processOnClicked() {
    if (m_correction != NULL) {
        QImage ori_image;
        QImage h_image;
        QImage v_image;
        QImage smooth_image;
        QImage strecth_image;
        QImage rotate_image;
        m_correction->process1(&ori_image, &rotate_image, &h_image, &v_image,&smooth_image, &strecth_image);
        ui->label_original_image->setPixmap(QPixmap::fromImage(ori_image));
        ui->label_h_image->setPixmap(QPixmap::fromImage(h_image));
        ui->label_v_image->setPixmap(QPixmap::fromImage(v_image));
        ui->label_smooth_image->setPixmap(QPixmap::fromImage(smooth_image));
#if 1
        ui->label_strecth_image->setPixmap(QPixmap::fromImage(strecth_image));
#endif
        ui->label_rotate_image->setPixmap(QPixmap::fromImage(rotate_image));
        ui->tabWidget->show();
    }
}
