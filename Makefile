#
# curvr
# Richard Crowley <r@rcrowley.org>
#

INCLUDE := -I/usr/local/include/GraphicsMagick -I/usr/local/include/exiv2
LIB := -L/usr/local/lib \
	-lGraphicsMagick++ -lGraphicsMagick \
	-lexiv2 \
	-lz -lbz2 \
	-lxml2 -lexpat \
	-ljpeg -ltiff

FLAGS := -O2 -Wall

all: depth curvr

curvr:
	g++ $(FLAGS) -DQUANTUM_DEPTH_`./depth` -o curvr curvr.cc \
		$(INCLUDE) $(LIB)

depth:
	g++ $(FLAGS) -o depth depth.cc $(INCLUDE) $(LIB)

install:
	cp curvr ~/bin/
	cp curvall ~/bin/
	cp curvrmail ~/bin/
	cp curvrconf ~/bin/

clean:
	rm -f curvr depth
