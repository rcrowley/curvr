/* curvr
 * curvr.cc
 * Richard Crowley
 * $Id: curvr.cc 22 2008-02-05 04:33:21Z rcrowley $
 */

// GraphicsMagick
#include "Magick++.h"

// Exiv2
#include "image.hpp"
#include "exif.hpp"
#include "iptc.hpp"

// Regular ass C++
#include <stdio.h>
#include <string>
#include <map>

/* Color maps
 *   Loosely based on Magick::PixelPacket structure.
 *   Notice that neither of these are #define'd anywhere in this file.
 *   You have to pass the -DQUANTUM_DEPTH_? flag to g++, but the Makefile
 *   does this for you using the depth.cc program.
 */
#ifdef QUANTUM_DEPTH_8
typedef unsigned char pixel_t;
typedef short cmap_channel_t;
typedef unsigned char range_t;
#elif QUANTUM_DEPTH_16
typedef unsigned short pixel_t;
typedef int cmap_channel_t;
typedef unsigned short range_t;
#endif
typedef struct _cmap_t {
	cmap_channel_t red;
	cmap_channel_t green;
	cmap_channel_t blue;
} cmap_t;

/* Build a color map, returning a pointer to the map
 *   Taking edge and middle, this creates a bowed shape across the entire
 *   color range, being == edge at both ends and approaching middle in
 *   the middle.
 */
cmap_t * build_cmap(int edge, int middle) {
	range_t range = ~0 >> 2;
	cmap_t * cmap = new cmap_t[range];

	// Map each color level in this quantum size
	for (range_t i = 0; i < range; ++i) {
		double halfway = (double)range / 2;
		double percent;
		if (i > halfway) {
			percent = 1.0 - ((double)i - (double)halfway) / halfway;
		} else {
			percent = (double)i / halfway;
		}

		// Could round here instead of truncating
		cmap_channel_t offset = (cmap_channel_t)((double)edge + percent *
			(double)middle);

		cmap[i].red = offset;
		cmap[i].green = offset;
		cmap[i].blue = offset;
	}
	return cmap;
}

/* Apply a color map to an image
 *   Uses the assumption that the red channel of an image is a good
 *   indicator of color value, so the map is of red values to a modifier
 *   structure.  Thus far, this assumption has been quite good.
 */
void apply_cmap(Magick::Image & img, cmap_t * cmap) {
	unsigned int columns = img.columns(), rows = img.rows();
	range_t range = ~0;
	Magick::Pixels view(img);

	// Map each pixel
	Magick::PixelPacket * px = view.get(0, 0, columns, rows);
	for (unsigned int c = 0; c < columns; ++c) {
		for (unsigned int r = 0; r < rows; ++r) {
			pixel_t red = px->red, green = px->green, blue = px->blue;

			/* Each color has bounds checking to prevent a negative offset
			 * from wrapping it down through zero and a positive offset
			 * from wrapping it over the top.  Empirically, favoring "on"
			 * for red seems to prevent the "aqua distortion" seen at
			 * http://flickr.com/photos/rcrowley/2233623053/.
			 */

			/* WTF?
			 */

			if (0 > cmap[red].red && red < -cmap[red].red) {
//				px->red = 0;
			} else if (0 < cmap[red].red && range - red < cmap[red].red) {
//				px->red = range;
			} else {
				px->red += cmap[red].red;
			}

			if (0 > cmap[red].green && green < -cmap[red].green) {
//				px->green = 0;
			} else if (0 < cmap[red].green && range - green < cmap[red].green) {
//				px->green = range;
			} else {
				px->green += cmap[red].green;
			}

			if (0 > cmap[red].blue && blue < -cmap[red].blue) {
//				px->blue = 0;
			} else if (0 < cmap[red].blue && range - blue < cmap[red].blue) {
//				px->blue = range;
			} else {
				px->blue += cmap[red].blue;
			}

			++px;
		}
	}

	view.sync();
}

/* Curve, bigcurve and anticurve processes
 *   Normalize the image and then color map it to be slightly darker.
 *   TODO: Make these parameterizable
 */
int _curve(Magick::Image & img, const char * process, int edge, int middle) {

	// Tuck white and black points
	printf("[%s] Normalizing\n", process);
	img.normalize();

	// A little bit darker now
	printf("[%s] Applying curve over [%d, %d)\n", process, edge, middle);
	cmap_t * cmap = build_cmap(edge, middle);
	apply_cmap(img, cmap);
	delete [] cmap;

	return 0;
}
int curve(Magick::Image & img) {
	return _curve(img, "curve", 0, -50);
}
int bigcurve(Magick::Image & img) {
	return _curve(img, "bigcurve", 0, -80);
}
int anticurve(Magick::Image & img) {
	return _curve(img, "anticurve", 0, 50);
}

int main(int argc, char * * argv) {

	// Gotta have two arguments, an input file and an output file
	std::string input = "", process = "curve", output = "";
	if (4 == argc) {
		input = *(argv + 1);
		process = *(argv + 2);
		output = *(argv + 3);
	} else if (3 == argc) {
		printf("[status] No process found, using default (%s)\n",
			process.c_str());
		input = *(argv + 1);
		output = *(argv + 2);
	} else {
		printf("Usage: %s <input> [<process>] <output>\n", *argv);
		printf("\tcurve\n\tbigcurve\n\tanticurve\n");
		return 1;
	}
	printf("[status] Input: %s, output: %s\n", input.c_str(), output.c_str());

	// Open the victim
	Magick::Image img;
	try {
		img = Magick::Image(input);
	} catch (Magick::Exception & e) {
		printf("[error] %s: %s\n", input.c_str(), e.what());
		return 2;
	}

	// Pull out EXIF and IPTC data for later
	Exiv2::ExifData exif;
	Exiv2::IptcData iptc;
	bool meta_found = true;
	try {
		Exiv2::Image::AutoPtr meta_r = Exiv2::ImageFactory::open(input);
		meta_r->readMetadata();
		exif = meta_r->exifData();
		iptc = meta_r->iptcData();
	} catch (Exiv2::Error &) {
		meta_found = false;
	}

	// Run the transform
	std::map<std::string, int (*)(Magick::Image &)> processes;
	processes["curve"] = &curve;
	processes["bigcurve"] = &bigcurve;
	processes["anticurve"] = &anticurve;
	printf("[status] Process: %s\n", process.c_str());
	(*processes[process])(img);

	// Write back to the output file
	img.compressType(Magick::NoCompression);
	img.write(output);

	// It's later, put back the EXIF and IPTC data
	try {
		Exiv2::Image::AutoPtr meta_w = Exiv2::ImageFactory::open(output);
		meta_w->setExifData(exif);
		meta_w->setIptcData(iptc);
		meta_w->writeMetadata();
	} catch (Exiv2::Error & e) {
		if (meta_found) printf("[error] exiv2: %s", e.what());
	}

	return 0;
}
