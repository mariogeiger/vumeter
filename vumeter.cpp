#include "vumeter.h"
#include <math.h>
#include <QShortcut>

Vumeter::Vumeter(QWidget *parent)
	: QGLWidget(parent)
{
	int rate = 192000 / 2;
	int moy = 1;
	video = 40;

	thread = new AlsaListen(this, rate, moy);
	thread->start();

	speed = rate / moy * video / 1000;

	timer = startTimer(video);

	new QShortcut(QKeySequence("Space"), this, SLOT(pause()));
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

void Vumeter::paintGL()
{
	thread->mutex.lock();
	if (thread->values().size() > width()) {
		glClear(GL_COLOR_BUFFER_BIT);
		glLoadIdentity();

		glBegin(GL_LINE_STRIP);

		double heightf = (double)height();
		double offset = heightf * 0.5;
		double factor = heightf / 8.0;

		for (int i = 0; i < width(); ++i) {
			float y = -thread->values().at(i) * factor + offset;

			float hue = y / heightf;
			QColor c = QColor::fromHsvF(hue, 1.0, 1.0);
			glColor3f(c.redF(), c.greenF(), c.blueF());

			glVertex2f(i, y);
		}

		for (int i = 0; i < speed && !thread->values().isEmpty(); ++i) {
			thread->values().removeFirst();
		}

		glEnd();
		glFlush();
	}
	thread->mutex.unlock();
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




AlsaListen::AlsaListen(QObject *parent, int rate, int moy)
	: QThread(parent), rate(rate), moy(moy)
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
	snd_pcm_hw_params_set_channels(handle, params, 1);
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

	size = frames * 1;
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

	float sum = 0;
	int count = 0;

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

		for (int i = 0; i < size; ++i) {
			sum += buffer[i];
			count++;

			if (count >= moy) {
				count = 0;
				mutex.lock();
				datas.append(((sum > 0.0) ? 1.0 : -1.0) * pow(sum / moy, 2));
				mutex.unlock();
				sum = 0;
			}
		}
	}
}

