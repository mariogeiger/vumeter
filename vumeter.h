#ifndef VUMETER_H
#define VUMETER_H

#include <QtGui/QWidget>
#include <QtOpenGL>

#include "alsalisten.h"

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

#endif // VUMETER_H
