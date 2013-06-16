#ifndef VUMETER_H
#define VUMETER_H

#include <QWidget>
#include <QtOpenGL>
#include <fftw3.h>

#include "alsalisten.h"

class Vumeter : public QGLWidget
{
    Q_OBJECT

public:
    Vumeter(QWidget *parent = 0);
    ~Vumeter();

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    virtual void timerEvent(QTimerEvent *);

private slots:
    void pause();
    void fullscreen();

private:
    void drawChanel(QList<float> &chanel, QColor color);

    AlsaListen *thread;
    int speed;
    int nc;
    int timer;
    int video;
    uint rate;

    double *in;
    fftw_complex *out;
    double *spectrum;
    fftw_plan plan;
};

#endif // VUMETER_H
