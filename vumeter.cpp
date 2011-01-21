#include "vumeter.h"
#include <math.h>
#include <QShortcut>

Vumeter::Vumeter(QWidget *parent)
	: QGLWidget(parent)
{
	int rate = 192000;
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

void Vumeter::drawChanel(QList<float> &chanel)
{
	thread->mutex.lock();
	if (chanel.size() > width()) {

		glBegin(GL_LINE_STRIP);

		double heightf = (double)height();
		double offset = heightf * 0.5;
		double factor = heightf / 8.0;

		for (int i = 0; i < width(); ++i) {
			float y = -chanel.at(i) * factor + offset;

			float hue = y / heightf;
			QColor c = QColor::fromHsvF(hue, 1.0, 1.0);
			glColor3f(c.redF(), c.greenF(), c.blueF());

			glVertex2f(i, y);
		}

		for (int i = 0; i < speed && !chanel.isEmpty(); ++i) {
			chanel.removeFirst();
		}

		glEnd();
		glFlush();
	}
	thread->mutex.unlock();
}

void Vumeter::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT);
	glLoadIdentity();

	drawChanel(thread->left());
	drawChanel(thread->right());
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




AlsaListen::AlsaListen(QObject *parent, int rate)
	: QThread(parent), rate(rate)
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
	uint val = rate;
	snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
	frames = 100000;
	snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	rc = snd_pcm_hw_params(handle, params);

	if (rc < 0) {
		fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
		exit(1);
	}

	snd_pcm_hw_params_get_period_size(params, &frames, &dir);

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
	}
}

