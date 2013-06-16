#include "vumeter.h"
#include <math.h>
#include <QShortcut>

Vumeter::Vumeter(QWidget *parent)
    : QGLWidget(parent)
{
    rate = 96000;
    video = 200;

    thread = new AlsaListen(this, rate, "default"); // plughw
    thread->start();

    speed = rate * video / 1000;
    nc = (speed / 2 ) + 1;

    in = new double[speed];
    out = (fftw_complex *)fftw_malloc(sizeof (fftw_complex) * nc);
    spectrum = new double[nc];

    plan = fftw_plan_dft_r2c_1d(speed, in, out, FFTW_ESTIMATE);

    timer = startTimer(video);

    new QShortcut(QKeySequence("Space"), this, SLOT(pause()));
    new QShortcut(QKeySequence("Ctrl+F"), this, SLOT(fullscreen()));
}

Vumeter::~Vumeter()
{
    thread->stop();
    thread->wait();

    fftw_destroy_plan(plan);

    delete[] spectrum;
    delete[] out;
    delete[] in;
}

void Vumeter::initializeGL()
{
    glDisable(GL_DEPTH_TEST);
}

void Vumeter::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity ();
    glOrtho(0, w, h, 0, 0, 1);
    glMatrixMode(GL_MODELVIEW);
}

void Vumeter::drawChanel(QList<float> &chanel, QColor color)
{
    if (chanel.size() >= speed) {
        thread->mutex.lock();

        for (int i = 0; i < speed; ++i) {
            in[i] = chanel.takeFirst();
        }
        thread->mutex.unlock();

        fftw_execute(plan);


        for (int i = 0; i < nc; ++i)
            spectrum[i] = out[i][0] * out[i][0] + out[i][1] * out[i][1];

        double max = 0.0;
        double fmax = 0.0;
        for (int i = 1; i < nc; ++i) {
            if (spectrum[i] > max) {
                max = spectrum[i];
                fmax = double(i);
            }
        }
        qDebug("%fHz (%d samples)", fmax * double(rate) / double(speed), speed);

        double dx = ((double)width() + 1.0) / log10(nc - 9);

        glBegin(GL_LINE_STRIP);
        glColor3d(color.redF(), color.greenF(), color.blueF());
        for (int i = 9; i < nc; ++i) {
            double x = dx * log10(i - 8);
            double y = (double)height() * (1.0 - spectrum[i] / max);
            glVertex2d(x, y);
        }
        glEnd();
    }
}

void Vumeter::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

//    drawChanel(thread->getleft(), QColor(Qt::red));
    drawChanel(thread->getright(), QColor(Qt::white));
}

void Vumeter::timerEvent(QTimerEvent *)
{
    updateGL();
}

void Vumeter::pause()
{
    if (thread->isRunning()) {
        thread->stop();
        killTimer(timer); timer = 0;
    } else {
        thread->start();
        timer = startTimer(video);
    }
}

void Vumeter::fullscreen()
{
    this->setWindowState(this->windowState() ^ Qt::WindowFullScreen);
}
