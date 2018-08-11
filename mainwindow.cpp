#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QImage>

QString sDefaultFile = "C:/Debian_WorkSpace/fisheye_distortion/build/bw_rect_168.bmp";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mCorrection = FisheyeDistortionCorrection::getInstance();
    ui->edit_file_location->setText(sDefaultFile);
    mCorrection->SetFileLocation(sDefaultFile);
    mOriginalImage = mCorrection->GetDefaultImage();
    if (false == mOriginalImage.isNull())
    {
        ui->label_original_image->setPixmap(QPixmap::fromImage(mOriginalImage));
        ui->edit_width->setText(QString::number(mOriginalImage.width()));
        ui->edit_height->setText(QString::number(mOriginalImage.height()));
    }
    ui->tabWidget->setCurrentIndex(0);
    QWidget::showMaximized();
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
    if (mCorrection != NULL) {
        qDebug() << "filepath = " << filepath;
        ui->edit_file_location->setText(filepath);
        mCorrection->SetFileLocation(filepath);
        mOriginalImage = mCorrection->GetDefaultImage();
    }
    if (false == mOriginalImage.isNull())
    {
        ui->label_original_image->setPixmap(QPixmap::fromImage(mOriginalImage));
        ui->edit_width->setText(QString::number(mOriginalImage.width()));
        ui->edit_height->setText(QString::number(mOriginalImage.height()));
    }

}

void MainWindow::processOnClicked() {
    if (mCorrection != NULL) {
        int width           = ui->edit_width->text().toInt();
        int height          = ui->edit_height->text().toInt();
        int opticalCenterW  = ui->edit_opt_center_x->text().toInt();
        int opticalCenterH  = ui->edit_opt_center_y->text().toInt();
        int rotation        = ui->edit_rotation->text().toInt();
        int cropX           = ui->edit_crop_x->text().toInt();
        int cropY           = ui->edit_crop_y->text().toInt();
        int cropW           = ui->edit_crop_w->text().toInt();
        int cropH           = ui->edit_crop_h->text().toInt();
        int hBase           = ui->edit_h_base->text().toInt();
        int vBase           = ui->edit_v_base->text().toInt();

        mCorrection->SetPictureSize(width, height);
        mCorrection->SetOpticalCenterPoint(opticalCenterW, opticalCenterH);
        mCorrection->SetRotation(rotation);
        mCorrection->SetCrop(cropX, cropY, cropW, cropH);
        mCorrection->Set2rdCurveCoff(hBase, vBase);
        QImage h_image;
        QImage v_image;
        QImage smooth_image;
        QImage strecth_image;
        QImage rotate_image;
        mCorrection->Process3(&mOriginalImage, &rotate_image, &h_image, &v_image,&smooth_image, &strecth_image);
        ui->label_h_image->setPixmap(QPixmap::fromImage(h_image));
        ui->label_v_image->setPixmap(QPixmap::fromImage(v_image));
        ui->label_smooth_image->setPixmap(QPixmap::fromImage(smooth_image));
#if 1
        ui->label_strecth_image->setPixmap(QPixmap::fromImage(strecth_image));
#endif
        ui->label_rotate_image->setPixmap(QPixmap::fromImage(rotate_image));

        strecth_image.save("./test1.jpg");
        ui->tabWidget->show();
    }
}
