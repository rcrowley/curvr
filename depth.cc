/*
 * curvr
 * Richard Crowley <r@rcrowley.org>
 *
 * This is a test file to determine the quantum depth GraphicsMagick uses.
 */

#include "Magick++.h"
#include <stdio.h>

using namespace Magick;

int main(int, char * *) {
	Geometry geo(1, 1);
	Color color("red");
	Image img(geo, color);
	printf("%d", img.depth());
}
