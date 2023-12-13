PRJNAME = m32-pars-adapt
SRCDIR = ../source
BLDDIR = ./build
TARGET = $(BLDDIR)/$(PRJNAME)
VPATH := $(SRCDIR)
MAIN = $(SRCDIR)/main.cpp
HEADERS = $(wildcard $(SRCDIR)/*.hpp)

CXX = g++
CXXFLAGS = -std=c++2b -funsigned-char -Wall -Wextra -Wpedantic -Wconversion -Wfatal-errors -O3 -lfmt -I$(SRCDIR)
#MSVCFLAGS = cl.exe /std:c++latest /utf-8 /J /W4 /O2


all: executable

debug: CXXFLAGS += -DDEBUG -D_DEBUG -g
debug: executable

executable: $(MAIN) $(HEADERS) makefile
	$(CXX) -o $(TARGET) $(CXXFLAGS) $(MAIN)


clean:
	rm $(BLDDIR)/*.o
