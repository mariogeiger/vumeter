#include "vumeter.h"
#include <math.h>
#include <QShortcut>

Vumeter::Vumeter(QWidget *parent)
	: QGLWidget(parent)
{
	uint rate = 192000;
	video = 40;

	thread = new AlsaListen(this, rate);
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
	while (chanel.size() > width()) {
		if (lastWasNegativ && chanel.first() > 0.0)
			break;

		if (chanel.first() < 0.0)
			lastWasNegativ = true;

		chanel.removeFirst();
	}
	thread->mutex.unlock();

	if (chanel.size() > width()) {

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

		for (int i = 0; i < speed && !chanel.isEmpty(); ++i) {
			chanel.removeFirst();
		}
		if (chanel.size() > thread->getrate()) {
			chanel.clear();
			qDebug("clear : rate = %d bps", thread->getrate());
		}
		thread->mutex.unlock();

		glEnd();
		glFlush();
	}
}

void Vumeter::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

	//qDebug("%d", thread->getleft().size());

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




AlsaListen::AlsaListen(QObject *parent, uint &_rate)
	: QThread(parent), rate(_rate)
{
	int rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);

	if (rc < 0) {
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}

	snd_pcm_hw_params_t *params;
	int dir;

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(handle, params);
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_NONINTERLEAVED);
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_FLOAT_LE);
	snd_pcm_hw_params_set_channels(handle, params, 2);

	snd_pcm_hw_params_set_rate_near(handle, params, &rate, &dir);
	snd_pcm_hw_params_get_rate(params, &rate, &dir);
	_rate = rate;

	frames = rate / 100;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
	snd_pcm_hw_params_get_period_size(params, &frames, &dir);

	rc = snd_pcm_hw_params(handle, params);

	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
		exit(1);
	}


	size = frames * 2;
	buffer = (float *)malloc(size * 4);
}

AlsaListen::~AlsaListen()
{
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
}

void AlsaListen::run()
{
	stopnext = false;

	while (!stopnext) {
		int rc = snd_pcm_readi(handle, buffer, frames);

		if (rc == -EPIPE) {
			fprintf(stderr, "overrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
		} else if (rc != (int)frames) {
			fprintf(stderr, "short read, read %d frames\n", rc);
		}

		for (int i = 0; i < size; i += 2) {
			mutex.lock();
			left.append(buffer[i]);
			right.append(buffer[i + 1]);
			mutex.unlock();
		}
		//qDebug("new frame: %d - %d", left.size(), right.size());
	}
}

