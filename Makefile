CC:=c++		# add starting from binutils-2.22
LDFLAGS:=$(shell pkg-config --cflags --libs opencv) -lavutil -lavcodec -lavformat -lswscale -lboost_system -lboost_thread -lpthread -lX11
CXXFLAGS:=-I/usr/include/opencv -I/usr/include/opencv2 -std=c++0x -O3
.SUFFIX:.o .cc

# excecutable and documentation
TARGET:=VideoAnalyzer
TARGET2:=/tmp/na
SRC:=$(TARGET).cc
OBJ=$(TARGET).o

all:$(TARGET)
$(TARGET):$(OBJ) hist.o sketch.o conf.o encoder.o dynan.o 
$(TARGET2):$(TARGET) Doxyfile
	doxygen
$(OBJ): cv_templates.hh

cv_templates.hh: cv_templates.cc
	@-touch dynan.cc conf.cc
hist.o: hist.hh hist.cc
dynan.o:dynan.hh dynan.cc
sketch.o:sketch.hh sketch.cc
conf.o:conf.hh conf.cc
encoder.o:encoder.hh encoder.cc
.PHONY:clean
clean:
	rm -f *.o $(TARGET)
