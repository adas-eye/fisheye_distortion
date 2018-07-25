#ifndef FisheyeDistortionCorrection_H
#define FisheyeDistortionCorrection_H

#include <QString>
#include <QImage>
class FisheyeDistortionCorrection
{
public:
    static  FisheyeDistortionCorrection * getInstance();
    void    SetPictureSize(int width, int height);
    void    SetFileLocation(QString path);
    void    SetOpticalCenterPoint(int x, int y);
    void    Set2rdCurveCoff(int hBase, int vBase);
    void    SetRotation(int rotation);
    void    SetCrop(int x, int y, int w, int h);
    void    Process(QImage *ori_image, QImage *h_image,
            QImage *v_image, QImage *smooth_image, QImage *strecth_image);
    void    Process1(QImage *oriImage, QImage *hImage,
            QImage *rotateImage, QImage *vImage, QImage *smoothImage, QImage *strecthImage);

    QImage  GetDefaultImage();
    QImage  DoImageRotate(QImage *image, int angleValue);
    /*
     * GetArchLens() : get the arc length between the (x0, y0) and (x1, y1).
     * a, b, c are the two-order curve cofficients.
     * the first point : x0, y0.
     * the second point: x1, y1.
     **/
    int     GetArchLens(float a, float b, float c, int x0, int y0, int x1, int y1);

    float   GetArchLens(float a, float  b, float c, int x0, int x1);

    int     AlignTo(int value, int k);

    int     MyMin(int a, int b)
    {
        return (a > b) ? b : a;
    }

    int     MyMax(int a, int b)
    {
        return (a > b) ? a : b;
    }

    int     Range(int value, int min, int max)
    {
        if ( value < min) return min;
        if ( value > max) return max;
        return value;
    }
private:
    FisheyeDistortionCorrection();
    void Initialize();

    QString     mFilePath;
    int         mWidth;
    int         mHeight;
    int         mOpticalCenterX;
    int         mOpticalCenterY;
    int         mRotation;
    int         mHorizontalBase;
    int         mVerticalBase;
    int         mCropX;
    int         mCropY;
    int         mCropW;
    int         mCropH;
};

#endif // FisheyeDistortionCorrection_H