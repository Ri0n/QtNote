#include "utils.h"

Utils::Utils()
{
}

QString Utils::cuttedDots(const QString &src, int length)
{
	Q_ASSERT(length > 3);
	if (src.length() > length) {
		return src.left(length - 3) + "...";
	}
	return src;
}