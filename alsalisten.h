#ifndef ALSALISTEN_H
#define ALSALISTEN_H

#include <QThread>
#include <QMutex>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

class AlsaListen : public QThread
{
	Q_OBJECT

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

#endif // ALSALISTEN_H
