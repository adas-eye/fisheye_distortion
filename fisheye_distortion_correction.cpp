#include "fisheye_distortion_correction.h"

#include <QImage>
#include <QDebug>
#include <qmath.h>

//#define PARABOLIC
#define CIRCEL
//#define ELLIPSE
fisheye_distortion_correction::fisheye_distortion_correction()
{
    initialize();
}

fisheye_distortion_correction *fisheye_distortion_correction::getInstance() {
   static fisheye_distortion_correction correction;
   return &correction;
}

void fisheye_distortion_correction::initialize() {
    setPictureSize(0,0);
    setFileFormat(0);
    setCorrectionCofficient(0, 100);
    setFileLocation(NULL);
}

void fisheye_distortion_correction::setFileFormat(int format) {
    this->format = format;
}

void fisheye_distortion_correction::setPictureSize(int width, int height) {
    this->width = width;
    this->height = height;
}

void fisheye_distortion_correction::setFileLocation(QString filepath) {
    this->file_path = filepath;
}

void fisheye_distortion_correction::setCorrectionCofficient(int coff_value, int coff_range) {
    this->coff_value = coff_value;
    this->coff_range = coff_range;
}

QImage fisheye_distortion_correction::imageRotate(QImage *image, int angleValue)
{
    QMatrix matrix;
    matrix.rotate(angleValue);
    return image->transformed(matrix, Qt::FastTransformation);
}

void fisheye_distortion_correction::process1(QImage *oriImage, QImage *rotateImage, QImage *hImage, QImage *vImage, QImage *smoothImage, QImage *strecthImage)
{
    QImage image(file_path);
    if (image.isNull())
    {
        qDebug() << "bad image input";
        return;
    }

    *oriImage = image.convertToFormat(QImage::Format_RGB888);
    const int width = oriImage->width();
    const int height = oriImage->height();
    qDebug("original image size = %dx%d", width, height);

    *rotateImage = imageRotate(oriImage, 7);

    /**
     * do horizontal correction, two-order curve.
     * suspect the opitial pointer: (w/2, h/2).
     * the coordinate system: x aix <---> width; y aix <---> height.
     * the equation: y = a*x^x + b*x + c.
     * three point should be:
     *  (0, coff_h), (w/2, y'), (w, coff_h).
     * here, the y' should be changed according to the peak of the curve.
     */
    qDebug("original image size = %dx%d", width, height);

    //int mapX[width][height] = { 0 };
    int maxHorizontalArcLengh   = alignTo(width * M_PI_2, 2);
    QPoint *mappedX             = new QPoint[maxHorizontalArcLengh * height];

    const int arcSizeX          = width / 2;
    float *arcLengthDeltaX      = new float[arcSizeX];

    const int coff_h_base =     height / 2;

    for (int h = 0; h < height / 2; ++h)
    {
        /**
         * the euqtion should locate on these three points.
         * (0, coff_h), (width/2, h), (width, coff_h)
         * 1: coff_h = a * 0 * 0 + b * 0 + c
         * 2: h = a * (width / 2) * (width / 2) + b * (width / 2) + c
         * 3: coff_h = a * width * width + b * width + c.
         * then, we can calculate the coff: a , b , c.
         * here, the coff_h is tuneable value according the the h
         **/

        // h / (height / 2) = (coff_h - coff_h_base) / (height/2 - coff_h_base).
        float coff_h    = h * (height / 2 - coff_h_base) / (height / 2) + coff_h_base;

        float a         = 4 * (coff_h - h) / (float)width / (float) width;
        float b         = -a * width;
        float c         = coff_h;
        //qDebug("a = %f, b = %f, c = %f", a, b, c);
        for (int arc = 0; arc < arcSizeX; arc++)
        {
            arcLengthDeltaX[arc] = GetArchLens(a, b, c, arc, arc + 1);
            //qDebug("arcLengthDeltaX = %f", arcLengthDeltaX[arc]);
        }

        int start = 0;
        int curr  = 0;
        for (int w = 0; w < width; ++w)
        {
            int x0                  = w;
            int y0                  = a * x0 * x0 + b * x0 + c;
            int x0Flip              = w;
            int y0Flip              = height - 1 - y0;
            float arcLengthTotalX   = 0.0f;
            for (int arc = (w < arcSizeX) ? w : arcSizeX * 2 - w; arc < arcSizeX; ++arc)
            {
                arcLengthTotalX += arcLengthDeltaX[arc];
            }

            if (h == 0 && w == 0)
            {
                int arcLength = GetArchLens(a, b, c, x0, y0, width / 2, h);
                qDebug("the max arcLengthTotalX= %f, arcLength = %d", arcLengthTotalX, arcLength);
            }
            if ( x0 < width /2 ) {
                curr = maxHorizontalArcLengh / 2 - (int)arcLengthTotalX;
            }
            else
            {
                curr = maxHorizontalArcLengh / 2 + (int)arcLengthTotalX;
            }

            int baseX       = h * maxHorizontalArcLengh;
            int baseXFlip   = (height -1 - h) * maxHorizontalArcLengh;
            for (int k = start; k < curr; ++k)
            {
                mappedX[baseX + k].setX(x0);
                mappedX[baseX + k].setY(y0);
                mappedX[baseXFlip + k].setX(x0Flip);
                mappedX[baseXFlip + k].setY(y0Flip);
            }
            start = curr;
        }
    }

    QImage horizonCorrection(maxHorizontalArcLengh, height, QImage::Format_RGB888);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < maxHorizontalArcLengh; x++)
        {
            horizonCorrection.setPixel(x, y, rotateImage->pixel(mappedX[y* maxHorizontalArcLengh + x]));
        }
    }
    *hImage = horizonCorrection;

    /**
     * do horizontal correction, two-order curve.
     * suspect the opitial pointer: (w/2, h/2).
     * the coordinate system: x aix <---> height, y aix <---> width.
     * the equation: y = a*x^x + b*x + c.
     * three point should be:
     *  (0, coff_w), (height / 2, y'), (height, coff_w).
     * here, the y' should be changed according to the peak of the curve.
     *       the coff_w is the dynamic change according the picture view.
     */

#if 1
    int coff_v_base             = 100;
    //int mayY[width][height] = { 0 };
    int maxVerticalArcLengh     = alignTo(height * M_PI_2, 2);
    QPoint *mappedY             = new QPoint[maxHorizontalArcLengh * maxVerticalArcLengh];
    const int arcSizeY          = height / 2;
    float *arcLengthDeltaY      = new float[arcSizeY];
    qDebug("maxHorizontalArcLength = %d, maxVerticalArcLength = %d", maxHorizontalArcLengh, maxVerticalArcLengh);


    for (int w = 0; w < maxHorizontalArcLengh / 2; ++w)
    {
        /**
         * the equation should locate these three points.
         * (0, w1), (height /2, w), (height,  w1)
         * 1: w1 = a * 0 * 0 + b * 0 + c
         * 2: w = a * height / 2 * height / 2 + b * height / 2 + c
         * 3: w1 = a * height * height + b * height + c
         **/
        //  w1 / (width / 2 - baseW) = w / (width / 2)
        int coff_w      = w * (maxHorizontalArcLengh / 2 - coff_v_base) / (maxHorizontalArcLengh / 2) + coff_v_base;
        float a         = 4 * (coff_w - w) / (float)maxHorizontalArcLengh / (float) maxHorizontalArcLengh;
        float b         = a * maxHorizontalArcLengh;
        float c         = coff_w;
        //qDebug("a = %f, b = %f, c = %f", a, b, c);
        for (int arc = 0; arc < arcSizeY; arc++)
        {
            arcLengthDeltaY[arc] = GetArchLens(a, b, c, arc, arc + 1);
        }

        int start = 0;
        int curr  = 0;
        for (int h = 0; h < height; ++h)
        {
            int x0                  = h;
            int y0                  = a * x0 * x0 + b * x0 + c;
            int x0Flip              = h;
            int y0Flip              = maxHorizontalArcLengh - 1 - y0;
            float arcLengthTotalY    = 0.0f;
            for (int arc = (x0 < arcSizeY) ? x0 : (2 * arcSizeY - x0); arc < arcSizeY; ++arc)
            {
                arcLengthTotalY += arcLengthDeltaY[arc];
            }
            if ( h == 0 && w == 0)
            {
                int arcLength = GetArchLens(a, b, c, x0, y0, height / 2, w);
                qDebug("the max arcLength  = %d, arcLengthTotalY = %f", arcLength, arcLengthTotalY);
            }

            if ( x0 < height / 2)
            {
                curr = maxVerticalArcLengh / 2 - (int)arcLengthTotalY;
            }
            else
            {
                curr = maxVerticalArcLengh / 2 + (int)arcLengthTotalY;
            }
#if 1
            for ( int k = start; k < curr; ++k)
            {
                int offset = k * maxHorizontalArcLengh + w;
                int offsetFlip = k * maxHorizontalArcLengh + (maxHorizontalArcLengh -1 - w);
                mappedY[offset].setX(y0);
                mappedY[offset].setY(x0);
                mappedY[offsetFlip].setX(y0Flip);
                mappedY[offsetFlip].setY(x0Flip);
            }
#endif
            start = curr;
        }
    }
    QImage verticalCorrection(maxHorizontalArcLengh, maxVerticalArcLengh, QImage::Format_RGB888);
    for (int y = 0; y < maxVerticalArcLengh; y++)
    {
        for (int x = 0; x < maxHorizontalArcLengh; x++)
        {
            verticalCorrection.setPixel(x, y, hImage->pixel(mappedY[y * maxHorizontalArcLengh + x]));
        }
    }
    *vImage = verticalCorrection;
#endif
    int cropX0  = (maxHorizontalArcLengh - width) / 3;
    int cropY0  = (maxVerticalArcLengh - height) / 2;
    int cropW   = maxHorizontalArcLengh - cropX0;
    int cropH   = maxVerticalArcLengh - cropY0 - 150;
    qDebug("cropX =%d, cropY = %d, cropW = %d, cropH = %d", cropX0, cropY0, cropW, cropH);
    *smoothImage = verticalCorrection.copy(cropX0,cropY0, cropW, cropH);
    *strecthImage = smoothImage->scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    qDebug("strecth image wxh = %dx%d", strecthImage->width(), strecthImage->height());
}

float fisheye_distortion_correction::GetArchLens(float a, float b, float c, int x0, int x1)
{
    float arcLength = 0.0f;
    const float deltax = 0.1f;
    float fx0 = x0;
    float fx1 = x1;
    do
    {
        arcLength += qSqrt(1 + qPow(2 * a * fx0 + b, 2)) * deltax;
        fx0 += deltax;
    }
    while ( fx0 < fx1);
    return arcLength;
}

int fisheye_distortion_correction::GetArchLens(float a, float b, float c, int x0, int y0, int x1, int y1)
{
    float arcLength = 0.0f;
    if ( x0 == x1) return arcLength;

    // two-order curve is: y = a * x^2 + b * x + c;
    // the derivative equation: y = 2 * a * x + b.
    const float delatx = 0.2f;
    float fx0 = (x0 < x1) ? x0 : x1;
    float fy0 = (x0 < x1) ? y0 : y1;
    float fx1 = (x0 < x1) ? x1 : x0;
    float fy1 = (x0 < x1) ? y1 : y0;

    do
    {
        arcLength += qSqrt(1 + qPow(2 * a * fx0 + b, 2))  * delatx;
        fx0 += delatx;
    } while ( fx0 < fx1 + delatx);

    return arcLength;
}
void fisheye_distortion_correction::process(QImage *ori_image,
                                            QImage *h_image,
                                            QImage *v_image,
                                            QImage *smooth_image,
                                            QImage *strecth_image)
{
    QImage image(file_path);
    if (image.isNull()) {
        qDebug() << "bad image input";
        return ;
    }
    *ori_image= image.convertToFormat(QImage::Format_RGB888);
    const int w = ori_image->width();
    const int h = ori_image->height();
    qDebug() << "width = " << w;
    qDebug() << "height = " << h;

    *h_image = *ori_image;
#ifdef PARABOLIC
    // the parabolic is: y = a * (x -k) * (x -k) + c;
    // the parabolic will locate at :
    // point 1(0, coff_c);
    // point 2(w-1, coff_c);

    int y_end = h/2;
    int x_end = w;
    //suspect the coff_a coff_c changed by linerality by y index.
    //when y_index = 0; a = coff_a
    //when y_index = x_end; a = 0;

    // here the coff_a and coff_c is tunable.
    float coff_a = 0.0015f; //coff_value/ coff_range;

    // here the coff_c should be [0, y_end]
    float coff_c = 0;
    float k = w / 2.0f;

    for (int y = 0; y < y_end; y++) {
        float a = coff_a * ( 1 - y /(float) y_end);
        float c = y;
        qDebug() << "a = " << a;
        qDebug() << "c = " << c;
        for (int x = 0; x < x_end; x++) {
            int y1 = a * (x -k)* (x -k) + c;
            if (y1 > y_end) y1 = y_end;
            newRgb.setPixel(x,y, rgb.pixel(x, y1));
        }
    }
#elif defined(CIRCEL)
    // the correction is circle equation.
    // (x - a)^(x - a)  + (y - b)^(y - b) = r * r.
    float a = 0.0f;
    float b = 0.0f;
    float r = 0.0f;
    int x_end = 0.0f;
    int y_end = 0.0f;

    // for horizontal orientation:
    // the optical point is: (a, b), here the a is fixed value:
    //          a = w/2.0f;
    // and then suspect the circel will go through these three point:
    //          (0, coff_h), (w, coff_h), (a, y); ( y should be 0~b).
    // and resove the equation, we can get the b and r.

    a = (w-1) / 2.0f;

    // then:
    // a^a + (coff_h-b)^(coff_h-b) = r *r
    // (y-b) ^ (y -b) = r*r

    // here, the coff range is tuneable according to the eyefish picture.
    float coff_h_base = 200;
    x_end = w;
    y_end = (h-1) / 2.0;

    // suspec the coff_h match the function: coff = a_h* x^(gamma) + b_h
    // will go through this to point(0, coff_h_base/h_end), ( 1, 1)
    float b_h = coff_h_base/(float)y_end;
    float gamma_h = 1/1.12;
    QColor pre;
    QColor pre1;
    for (int y = 0; y < y_end; y++) {
#if 0
        float coff_h = y+ coff_h_base - y * coff_h_base / y_end;
#else
        float a_h = 1 - b_h;
        float coff_h = (a_h * pow(y/(float)y_end, gamma_h) + b_h) * y_end;
#endif
        b = (y * y - coff_h * coff_h - a * a) / (float)(2.0 * ( y - coff_h));
        r = sqrt((y-b) * (y-b));
        qDebug() << "a = " << a << ", b = " << b << ",r = " << r;
        for (int x = 0; x < x_end; x++) {
            // y = sqrt(r*r-(x-a)*(x-a));
            int y1 = (int)(b- sqrt(r * r - (x-a)*(x-a)) + 0.5f) ;
            if (y1 < 0) y1 = 0;
            if (y1 > y_end) y1= y_end;
            QColor color = ori_image->pixelColor(x,y1);
            if (x == 0) {
                pre = color;
                pre1 = color;
            }
            color.setRed((color.red() + pre.red() + pre1.red()) / 3 + 0.5f);
            color.setBlue((color.blue() + pre.blue() + pre1.blue()) / 3 + 0.5f);
            color.setGreen((color.green() + pre.green() + pre1.green()) / 3 + 0.5f);

            //h_image->setPixel(x, y, ori_image->pixel(x, y1));
            h_image->setPixelColor(x, y, color);
            pre1 = pre;
            pre = ori_image->pixelColor(x, y1);

            // for horizontal orientation, the picture are symmetric.
            // the symmetric aix is y = y_end;
            int y_mirror = h - y-1;
            if (y_mirror > h) y_mirror = h;
            if (y_mirror < 0) y_mirror = 0;
            int y_mirror1 = h - y1-1;
            if (y_mirror1 > h) y_mirror1 = h;
            if (y_mirror1 < 0) y_mirror1 = 0;
            h_image->setPixel(x, y_mirror, ori_image->pixel(x, y_mirror1));

        }
    }

    *v_image = *h_image;
    // for vertical orientation:
    // the circle point is: (a, b), here the b is fixed value:
    //          b = h / 2.0f;
    // and then suspect the circel will go through these three point:
    //         (x, b), (coff_v, 0) (coff, h)
    // and resove the equation, we can get the b and r.
    b = (h-1) / 2.0f;

    float coff_v_base = -200;
    x_end = (w-1) / 2.0f;
    y_end = h;

    // suspec the coff_v match the function: coff = a_v* x^(gamma) + b_v
    // will go through this to point(0, coff_v/x_end), ( 1, 1)
    float gamma_v = 1/2.4f;
    float b_v = coff_v_base/(float)x_end;
    float a_v = 1 - b_v;
    for (int x = 0; x < x_end; x++) {
#if 0
        float coff_v = coff_v_base - coff_v_base * x / x_end + x;
        //float coff_v = x + coff_v_base;
#else
        float coff_v = (a_v * pow (x/(float)x_end, gamma_v) + b_v) * x_end;

#endif
        if (coff_v > x_end) coff_v = x_end;
        float a = (x * x - coff_v * coff_v - b * b) / (2.0f * (x - coff_v));
        float r2 = (x -a) * (x - a);
        qDebug() <<"x =" << x << "coff_v = " << coff_v;
        qDebug() << "a = " << a << ", b = " << b << ",r = " << r;
        for (int y = 0; y < y_end; y++) {
            int x1 = int(a - sqrt(r2 - (y-b) * (y-b)) + 0.5f);
            if (x1 < 0) x1 = 0;
            if (x1 > x_end) x = x_end;
            if (coff_v < x) x1 = x;
            QColor color = h_image->pixelColor(x1, y);
            if (y == 0) {
                pre1 = color;
                pre = color;
            }
            color.setRed((color.red() + pre.red() + pre1.red()) / 3 + 0.5f);
            color.setBlue((color.blue() + pre.blue() + pre1.blue()) / 3 + 0.5f);
            color.setGreen((color.green() + pre.green() + pre1.green()) / 3 + 0.5f);

            //v_image->setPixel(x, y, h_image->pixel(x1, y));
            v_image->setPixelColor(x, y, color);
            pre1 = pre;
            pre = h_image->pixelColor(x1, y);
            // for vertical oriention, the picture still are symmetric
            int x_mirror = w-x-1;
            int x1_mirror = w-x1-1;
            v_image->setPixel(x_mirror, y, h_image->pixel(x1_mirror, y));
        }
    }
    v_image->save("v_image.jpg");
#elif defined(ELLIPSE)

#endif



    // improve the sawtooth.
    *smooth_image = *ori_image;
#if 0
    QImage gray = v_image->convertToFormat(QImage::Format_Grayscale8);
    int min_threshold = 60;
    int max_threshold = 140;
    int kernel_w = 3;
    int kernel_h = 3;
    int pixel[kernel_w][kernel_h];
    for (int y = 0; y < h-kernel_h; y++) {
        for (int x = 0; x < w-kernel_w; x++) {
            for (int y1 = 0; y1 < kernel_h ; y1++) {
                for (int x1 = 0; x1 < kernel_w; x1++) {
                    if (gray.pixelColor(x1+x, y1+y).value() > max_threshold) {
                        pixel[x1][y1] = 1;
                    } else if (gray.pixelColor(x1+x, y1+y).value() < min_threshold) {
                        pixel[x1][y1] = 0;
                    } else {
                        pixel[x1][y1] = -2;
                    }
                }
            }
            bool need_filter = false;
            do {
                int b0 = pixel[0][0] + pixel[0][1] + pixel[0][2];
                int b1 = pixel[0][2] + pixel[1][2] + pixel[2][2];
                int b2 = pixel[2][0] + pixel[2][1] + pixel[2][2];
                int b3 = pixel[0][0] + pixel[1][0] + pixel[2][0];
                if (pixel[1][1] == 0) {
                    if(b0+b1 == 6 || b1+b2 == 6 || b2+b3== 6 || b3+b0 == 6) {
                        need_filter = true;
                        break;
                    }

                }
                if (pixel[1][1] == 1) {
                    if (b0+b1 == 0 || b1+b2 == 0 || b2+b3== 0 || b3+b0 == 0){
                        need_filter = true;
                        break;
                    }
                }
            } while(0);
            if (need_filter == true) {
                QColor color;
                color.setRed((v_image->pixelColor(x,y).red()
                             + v_image->pixelColor(x+1, y).red()
                             + v_image->pixelColor(x+2, y).red()
                             + v_image->pixelColor(x,y+1).red()
                             + v_image->pixelColor(x+1,y+1).red()
                             + v_image->pixelColor(x+2,y+1).red()
                             + v_image->pixelColor(x, y+2).red()
                             + v_image->pixelColor(x+1, y+2).red()
                             + v_image->pixelColor(x+2, y+2).red())/9);
                color.setGreen((v_image->pixelColor(x,y).green()
                             + v_image->pixelColor(x+1, y).green()
                             + v_image->pixelColor(x+2, y).green()
                             + v_image->pixelColor(x,y+1).green()
                             + v_image->pixelColor(x+1,y+1).green()
                             + v_image->pixelColor(x+2,y+1).green()
                             + v_image->pixelColor(x, y+2).green()
                             + v_image->pixelColor(x+1, y+2).green()
                             + v_image->pixelColor(x+2, y+2).green())/9);
                color.setBlue((v_image->pixelColor(x,y).blue()
                             + v_image->pixelColor(x+1, y).blue()
                             + v_image->pixelColor(x+2, y).blue()
                             + v_image->pixelColor(x,y+1).blue()
                             + v_image->pixelColor(x+1,y+1).blue()
                             + v_image->pixelColor(x+2,y+1).blue()
                             + v_image->pixelColor(x, y+2).blue()
                             + v_image->pixelColor(x+1, y+2).blue()
                             + v_image->pixelColor(x+2, y+2).blue())/9);
#if 0
                //debug
                color.setRed(255);
                color.setGreen(64);
                color.setBlue(64);

#endif
                smooth_image->setPixelColor(x+1, y+1, color);
#if 0
                for (int y1 = 0; y1 < 3; y1++) {
                    for (int x1 = 0; x1 < 3; x1++) {
                        smooth_image->setPixelColor(x+x1, y+y1, color);
                    }
                }
#endif
            }
        }
    }
//#else
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int x1 = x;
            int x2= x+1;
            int y1 = y;
            int y2 = y+1;
            int x3 = x-1;
            int y3 = y-1;

            if (x2 >= w) x2 = w -1;
            if (y2 >= h) y2 = h -1;
            if (x3 < 0) x3 = 0;
            if (y3 < 0) y3 = 0;
            QColor color;
            color.setRed((v_image->pixelColor(x1, y1).red()
                          + v_image->pixelColor(x1,y2).red()
                          + v_image->pixelColor(x1, y3).red()
                          + v_image->pixelColor(x2, y1).red()
                          + v_image->pixelColor(x2, y2).red()
                          + v_image->pixelColor(x2, y3).red()
                          + v_image->pixelColor(x3,y1).red()
                          + v_image->pixelColor(x3, y2).red()
                          + v_image->pixelColor(x3, y3).red()) / 9);
            color.setGreen((v_image->pixelColor(x1, y1).green()
                            + v_image->pixelColor(x1,y2).green()
                            + v_image->pixelColor(x1, y3).green()
                            + v_image->pixelColor(x2, y1).green()
                            + v_image->pixelColor(x2, y2).green()
                            + v_image->pixelColor(x2, y3).green()
                            + v_image->pixelColor(x3,y1).green()
                            + v_image->pixelColor(x3, y2).green()
                            + v_image->pixelColor(x3, y3).green()) / 9);
            color.setBlue((v_image->pixelColor(x1, y1).blue()
                           + v_image->pixelColor(x1,y2).blue()
                           + v_image->pixelColor(x1, y3).blue()
                           + v_image->pixelColor(x2, y1).blue()
                           + v_image->pixelColor(x2, y2).blue()
                           + v_image->pixelColor(x2, y3).blue()
                           + v_image->pixelColor(x3,y1).blue()
                           + v_image->pixelColor(x3, y2).blue()
                           + v_image->pixelColor(x3, y3).blue()) / 9);
            smooth_image->setPixelColor(x, y, color);
        }
    }
#endif

    //according to the optical center point. we need do strection.
    const float strection_w_ratio               = M_PI_2;
    const float strection_h_ratio               = M_PI_2 * 0.8;
    const int strection_width                   = alignTo(w * strection_w_ratio * 2, 2);
    const int strection_height                  = alignTo(h * strection_h_ratio * 2, 2);
    const int ocw                               = w / 2;
    const int och                               = h / 2;
    const int ocsw                              = strection_width / 2;
    const int ocsh                              = strection_height / 2;

    qDebug("wxh=(%d, %d), strech wxh=(%d, %d)", w, h, strection_width, strection_height);
    QImage strection(strection_width, strection_height, QImage::Format_RGB888);
    for (int y = ocsh; y > 0; --y)
    {
        int y1 = ldcMin(h-1, qPow(y / (float)ocsh, 4.0) * och);
        for (int x = ocsw; x > 0; --x)
        {
            int x1 = ldcMin(w-1, qPow(x / (float)ocsw, 4.0) * ocw);

            strection.setPixel(x, y, smooth_image->pixel(x1, y1));

            strection.setPixel(2 * ocsw - x, y, smooth_image->pixel(2 * ocw - 1 - x1, y1));

            strection.setPixel(x, 2 * ocsh - y, smooth_image->pixel(x1, 2 * och - 1 - y1));

            strection.setPixel(2 * ocsw - x, 2 * ocsh - y, smooth_image->pixel(2 * ocw - 1 -x1, 2 * och - 1 - y1));
        }
    }
    *strecth_image = strection.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

int fisheye_distortion_correction::alignTo(int value, int k)
{
    int delat  = value % k;
    if (delat == 0) return value;
    return value + k - delat;
}
