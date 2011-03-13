#include "alsalisten.h"


AlsaListen::AlsaListen(QObject *parent, uint &_rate, const char *device)
    : QThread(parent), rate(_rate)
{
    snd_pcm_hw_params_t *params;


    int err;

    /* SND_PCM_STATE_OPEN */

    err = snd_pcm_open(&pcm, device, SND_PCM_STREAM_CAPTURE, /*SND_PCM_NONBLOCK*/0);

    if (err < 0) {
        pcm = 0;
        qDebug("snd_pcm_open: %s", snd_strerror(err));
        return;
    }

    snd_pcm_hw_params_alloca(&params);

    err = snd_pcm_hw_params_any(pcm, params);
    if (err < 0) {
        qDebug("snd_pcm_hw_params_any: %s", snd_strerror(err));
        return;
    }

    err = snd_pcm_hw_params_set_rate_resample(pcm, params, 1);
    if (err < 0) {
        qDebug("snd_pcm_hw_params_set_rate_resample: %s", snd_strerror(err));
        return;
    }

    err = snd_pcm_hw_params_set_access(pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        qDebug("snd_pcm_hw_params_set_access: %s", snd_strerror(err));
        return;
    }

    err = snd_pcm_hw_params_set_format(pcm, params, SND_PCM_FORMAT_FLOAT);
    if (err < 0) {
        qDebug("snd_pcm_hw_params_set_format: %s", snd_strerror(err));
        return;
    }

    err = snd_pcm_hw_params_set_channels(pcm, params, 2);
    if (err < 0) {
        qDebug("snd_pcm_hw_params_set_channels: %s", snd_strerror(err));
        return;
    }

    unsigned int ratevalue = rate;
    // ratevalue = 192000;
    // ratevalue = 96000;
    // ratevalue = 44100;
    err = snd_pcm_hw_params_set_rate_near(pcm, params, &ratevalue, 0);
    if (err < 0) {
        qDebug("snd_pcm_hw_params_set_rate_last: %s", snd_strerror(err));
        return;
    } else {
        qDebug("rate: %u", ratevalue);
    }


    period = ratevalue / 10;
    // period = 19200;
    // period = 9600;
    //    period = 4410;
    err = snd_pcm_hw_params_set_period_size_near(pcm, params, &period, 0);
    if (err < 0) {
        qDebug("snd_pcm_hw_params_set_period_size_near: %s", snd_strerror(err));
        return;
    }
    int dir;
    err = snd_pcm_hw_params_get_period_size(params, &period, &dir);
    if (err == 0) {
        qDebug("period: %ld", period);
    }

    snd_pcm_uframes_t buffersize = 4 * period;

    err = snd_pcm_hw_params_set_buffer_size_near(pcm, params, &buffersize);
    if (err < 0) {
        qDebug("snd_pcm_hw_params_set_buffer_size_near: %s", snd_strerror(err));
        return;
    }

    err = snd_pcm_hw_params_get_buffer_size(params, &buffersize);
    if (err == 0) {
        qDebug("buffersize: %lu", buffersize);
    }


    err = snd_pcm_hw_params(pcm, params);
    if (err < 0) {
        qDebug("snd_pcm_hw_params: %s", snd_strerror(err));
        return;
    }


    buffer = new float[2 * period];

    /* SND_PCM_STATE_SETUP */

    err = snd_pcm_prepare(pcm);
    if (err < 0) {
        qDebug("snd_pcm_prepare: %s", snd_strerror(err));
    }

    /* SND_PCM_STATE_PREPARE */

    err = snd_pcm_start(pcm);
    if (err < 0) {
        qDebug("snd_pcm_prepare: %s", snd_strerror(err));
    }
}

AlsaListen::~AlsaListen()
{
    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
    free(buffer);
}

void AlsaListen::run()
{
    if (pcm == 0 || buffer == 0)
        return;

    snd_pcm_state_t state = snd_pcm_state(pcm);
    switch (state) {
    case SND_PCM_STATE_OPEN:
        qDebug("Open");
        break;
    case SND_PCM_STATE_SETUP:
        qDebug("Setup installed");
        break;
    case SND_PCM_STATE_PREPARED:
        qDebug("Ready to start");
        break;
    case SND_PCM_STATE_RUNNING:
        qDebug("Running");
        break;
    case SND_PCM_STATE_XRUN:
        qDebug("Stopped: underrun (playback) or overrun (capture) detected");
        snd_pcm_recover(pcm, -EPIPE, 0);
        break;
    case SND_PCM_STATE_DRAINING:
        qDebug("Draining: running (playback) or stopped (capture)");
        break;
    case SND_PCM_STATE_PAUSED:
        qDebug("Paused");
        break;
    case SND_PCM_STATE_SUSPENDED:
        qDebug("Hardware is suspended (stop thread)");
        return;
    case SND_PCM_STATE_DISCONNECTED:
        qDebug("Hardware is disconnected (stop thread)");
        return;
    }

    int err;
    int size = 2 * period;


    stopnext = false;

    while (stopnext == false) {


        err = snd_pcm_readi(pcm, buffer, period);


        switch (err) {
        case -EPIPE:
            qDebug("%s", snd_strerror(err));
            snd_pcm_recover(pcm, err, 0);
            continue;
        case -EBADFD:
            qDebug("%s", snd_strerror(err));
            snd_pcm_recover(pcm, err, 0);
            continue;
        case -ESTRPIPE:
            qDebug("%s", snd_strerror(err));
            snd_pcm_recover(pcm, err, 0);
            continue;
        case -EBUSY:
            qDebug("%s", snd_strerror(err));
            snd_pcm_recover(pcm, err, 0);
            continue;
        case -EAGAIN:
            qDebug("%s", snd_strerror(err));
            continue;
        default:
            break;
        }

        for (int i = 0; i < size; i += 2) {
            mutex.lock();
            left.append(buffer[i]);
            right.append(buffer[i + 1]);
            mutex.unlock();
        }
    }
}

