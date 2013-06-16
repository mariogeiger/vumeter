#include <QApplication>
#include "vumeter.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Vumeter w;

	w.show();

    return a.exec();
}

