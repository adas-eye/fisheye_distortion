#ifndef FISHEYE_DISTORTION_CORRECTION_H
#define FISHEYE_DISTORTION_CORRECTION_H

#include <QString>
#include <QImage>
class fisheye_distortion_correction
{
public:
    static fisheye_distortion_correction * getInstance();
    void setPictureSize(int width, int height);
    void setFileLocation(QString path);
    void setFileFormat(int format);
    void setCorrectionCofficient(int coff_value, int coff_range);
    void process(QImage *ori_image, QImage *h_image, QImage *v_image, QImage *smooth_image, QImage *strecth_image);
    void process1(QImage *oriImage, QImage *hImage, QImage *rotateImage, QImage *vImage, QImage *smoothImage, QImage *strecthImage);

    QImage imageRotate(QImage *image, int angleValue);
    /*
     * GetArchLens() : get the arc length between the (x0, y0) and (x1, y1).
     * a, b, c are the two-order curve cofficients.
     * the first point : x0, y0.
     * the second point: x1, y1.
     **/
    int GetArchLens(float a, float b, float c, int x0, int y0, int x1, int y1);

    float GetArchLens(float a, float  b, float c, int x0, int x1);

    int alignTo(int value, int k);

    int ldcMin(int a, int b)
    {
        return (a > b) ? b : a;
    }
    int ldcMax(int a, int b)
    {
        return (a > b) ? a : b;
    }
private:
    fisheye_distortion_correction();
    void initialize();
    int width;
    int height;
    int format;
    int coff_value;
    int coff_range;
    QString file_path;
};

#endif // FISHEYE_DISTORTION_CORRECTION_H
