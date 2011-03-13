#include "vumeter.h"
#include <math.h>
#include <QShortcut>

Vumeter::Vumeter(QWidget *parent)
    : QGLWidget(parent)
{
    rate = 96000;
    video = 50;

    thread = new AlsaListen(this, rate, "default"); // plughw
    thread->start();

    speed = rate * video / 1000;

    in = new fftw_real[speed];
    out = new fftw_real[speed];
    spectrum = new fftw_real[speed / 2 + 1];

    plan = rfftw_create_plan(speed, FFTW_REAL_TO_COMPLEX, FFTW_ESTIMATE);

    timer = startTimer(video);

    new QShortcut(QKeySequence("Space"), this, SLOT(pause()));
    new QShortcut(QKeySequence("Ctrl+F"), this, SLOT(fullscreen()));
}

Vumeter::~Vumeter()
{
    thread->stop();
    thread->wait();

    rfftw_destroy_plan(plan);

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

        rfftw_one(plan, in, out);

        spectrum[0] = out[0] * out[0];  /* DC component */

        for (int k = 1; k < (speed+1)/2; ++k)  /* (k < N/2 rounded up) */
            spectrum[k] = out[k] * out[k] + out[speed-k] * out[speed-k];

        if (speed % 2 == 0) /* N is even */
            spectrum[speed/2] = out[speed/2] * out[speed/2];  /* Nyquist freq. */

        int n = speed / 2 + 1;

        double max = 0.0;
        int fmax = 0;
        for (int i = 0; i < n; ++i) {
            if (spectrum[i] > max) {
                max = spectrum[i];
                fmax = i;
            }
        }
        qDebug("%d", fmax * rate / speed);

        double dx = (double)width() / log10(n);

        glBegin(GL_LINE_STRIP);
        glColor3d(color.redF(), color.greenF(), color.blueF());
        for (int i = 0; i < n; ++i) {
            double x = dx * log10(i);
            //qDebug("s[%d] = %f", i, spectrum[i]);
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
