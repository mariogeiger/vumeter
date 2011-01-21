#ifndef VUMETER_H
#define VUMETER_H

#include <QtGui/QWidget>
#include <QtOpenGL>
#include <QThread>
#include <QMutex>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

class AlsaListen;

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
	int timer;
	int video;
};

class AlsaListen : public QThread
{
public:
	AlsaListen(QObject *parent, uint &rate);
	~AlsaListen();
	QList<float> &getleft() {return left;}
	QList<float> &getright() {return right;}
	void stop() {stopnext = true;}
	int getframes() const {return frames;}
	int getrate() const {return rate;}

	QMutex mutex;

private:
	void run();

	bool stopnext;
	float *buffer;
	int size;
	snd_pcm_t *handle;
	snd_pcm_uframes_t frames;
	uint rate;

	QList<float> left;
	QList<float> right;
};

#endif // VUMETER_H
