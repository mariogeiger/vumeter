#include "lockin.h"
#include <QList>

Lockin::Lockin()
{
}


QList<qreal> summits(const QList<qreal> &data, int period)
{
	QList<qreal> summits;
	qreal summit = data.first();
	int pos;
	int before = 0;
	int after = 0;

	for (int i = 1; i < data.size(); ++i) {

		if (data[i] > summit) {
			summit = data[i];
			pos = i;

			after = 0;
		} else if (after++ > period / 2) {
			before = 0;

			for (int j = pos - 1; j >= 0; --j) {
				if (data[j] < summit) {
					if (before++ > period / 2) {
						summits.append(summit);

						summit = data[i];
						pos = i;
						after = 0;

						break;
					}
				} else {
					summit = data[i];
					pos = i;
					after = 0;

					break;
				}
			}



		}



	}
}
