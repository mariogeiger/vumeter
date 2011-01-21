#include "alsalisten.h"


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
	}
}

