/* curvr
 * depth.cc
 * Richard Crowley
 * $Id: depth.cc 25 2008-02-05 04:33:44Z rcrowley $
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
