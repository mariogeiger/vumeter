#include "vumeter.h"
#include <math.h>
#include <QShortcut>

Vumeter::Vumeter(QWidget *parent)
	: QGLWidget(parent)
{
	uint rate = 192000;
	video = 40;

        thread = new AlsaListen(this, rate, "plughw:CARD=ICH5,DEV=3"); // plughw
	thread->start();

	speed = rate * video / 1000;

	timer = startTimer(video);

	new QShortcut(QKeySequence("Space"), this, SLOT(pause()));
	new QShortcut(QKeySequence("Ctrl+F"), this, SLOT(fullscreen()));
}

Vumeter::~Vumeter()
{
	thread->stop();
	thread->wait(5000);
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
	//! TRIGGER
	thread->mutex.lock();
	bool lastWasNegativ = false;
	int triggered;

	for (triggered = 0; chanel.size() > width(); triggered++) {
		if (lastWasNegativ && chanel[width() / 2] > 0.0)
			break;

		if (chanel[width() / 2] < 0.0)
			lastWasNegativ = true;

		chanel.removeFirst();
	}
	thread->mutex.unlock();

	if (chanel.size() >= width()) {

		glBegin(GL_LINE_STRIP);

		double min = 0;
		double max = 0;

		for (int i = 0; i < width(); ++i) {
			if (chanel[i] < min)
				min = chanel[i];
			if (chanel[i] > max)
				max = chanel[i];
		}

		min *= 1.05;
		max *= 1.05;
//		printf("%s [%f -> %f]\n",color.name().toAscii().data() , min, max);

		double heightf = (double)height();
		double delta = max - min;

		thread->mutex.lock();
		for (int i = 0; i < width(); ++i) {
			float y = heightf - ((chanel[i] - min) / delta) * heightf;

			//float hue = y / heightf;
			QColor c = color; //QColor::fromHsvF(hue, 1.0, 1.0);
			glColor3f(c.redF(), c.greenF(), c.blueF());

			glVertex2f(i, y);
		}

		for (int i = 0; i < speed - triggered && !chanel.isEmpty(); ++i) {
			chanel.removeFirst();
		}
		if (chanel.size() > thread->getrate()) {
			chanel.clear();
			qDebug("clear : rate = %d bps", thread->getrate());
		}
		thread->mutex.unlock();

		glEnd();
		glFlush();

		//qDebug("triggered : %d", triggered);
	} else {
                //qDebug("!(chanel.size() > width()) : %d", chanel.size());
	}
}

void Vumeter::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

//	qDebug("%d/%d", thread->getleft().size(), thread->getright().size());

	drawChanel(thread->getleft(), QColor(Qt::red));
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
