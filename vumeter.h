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

private:
	AlsaListen *thread;
	int speed;
	int timer;
	int video;
};

class AlsaListen : public QThread
{
public:
	AlsaListen(QObject *parent = 0, int rate = 10000, int moy = 1);
	~AlsaListen();
	QList<float> &values() {return datas;}
	void stop() {stopnext = true;}

	QMutex mutex;

private:
	void run();

	bool stopnext;
	float *buffer;
	int size;
	snd_pcm_t *handle;
	snd_pcm_uframes_t frames;
	int rate;
	int moy;

	QList<float> datas;
};

#endif // VUMETER_H
