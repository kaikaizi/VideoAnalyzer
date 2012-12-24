CC:=c++		# add starting from binutils-2.22
LDFLAGS:=$(shell pkg-config --cflags --libs opencv) -lX11 -std=c++0x
CXXFLAGS:=-I/usr/include/opencv -I/usr/include/opencv2 -std=c++0x -O3
.SUFFIX:.o .cc

# excecutable and documentation
TARGET:=VideoAnalyzer
TARGET2:=/tmp/na
SRC:=$(TARGET).cc
OBJ=$(TARGET).o

all:$(TARGET)
$(TARGET):$(OBJ) hist.o sketch.o dynan.o conf.o
$(TARGET2):$(TARGET) Doxyfile
	doxygen
$(OBJ): cv_templates.hh

cv_templates.hh: cv_templates.cc
	@-touch cv.cc dynan.cc conf.cc
hist.o: hist.hh hist.cc
dynan.o:dynan.hh dynan.cc
sketch.o:sketch.hh sketch.cc
conf.o:conf.hh conf.cc
.PHONY:clean
clean:
	rm -f *.o $(TARGET)
