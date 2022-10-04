PRJNAME = m32-pars-adapt
SRCDIR = ../source
BLDDIR = ./build
VPATH := $(SRCDIR)
MAIN = $(SRCDIR)/m32-pars-adapt.cpp
HEADERS = $(wildcard $(SRCDIR)/*.hpp)
#OBJS := $(HEADERS:.hpp=.o)
#OBJS := $(SRCS:%.cpp=BLDDIR/%.o)

CXX = clang++
CXXFLAGS = -std=c++2b -funsigned-char -Wall -Wextra -Wpedantic -Wconversion -Wfatal-errors -O3 -lfmt
#CXX = cl.exe
#CXXFLAGS = /std:c++latest /utf-8 /J /W4 /O2

ifeq ($(OS),Windows_NT)
	TARGET = $(BLDDIR)/$(PRJNAME).exe
else
	TARGET = $(BLDDIR)/$(PRJNAME)
endif


all: executable

debug: CXXFLAGS += -DDEBUG -D_DEBUG -g
debug: executable

executable: $(MAIN) $(HEADERS) makefile
	$(CXX) -o $(TARGET) $(CXXFLAGS) $(MAIN)

#my_program: $(OBJECTS)
#	$(CXX) $(OBJECTS) -o my_program
#
#processing.o: processing.cxx
#	$(CXX) $(INCLUDES) $(CXXFLAGS) -c processing.cxx -o processing.o


clean:
	rm *.o
