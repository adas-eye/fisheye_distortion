#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "fisheye_distortion_correction.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    fisheye_distortion_correction *m_correction;
public Q_SLOTS:
    void openFileOnClicked();
    void processOnClicked();
};

#endif // MAINWINDOW_H
