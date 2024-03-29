PRJNAME = m32-pars-adapt
SRCDIR = ../source
BLDDIR = ./bin
VPATH := $(SRCDIR)
CPPS = $(sort $(wildcard $(SRCDIR)/*.cpp $(SRCDIR)/*/*.cpp))
HEADERS = $(wildcard $(SRCDIR)/*.hpp $(SRCDIR)/*/*.hpp)
INCLUDEDIRS = $(sort $(SRCDIR) $(dir $(wildcard $(SRCDIR)/*/)))
TARGET = $(BLDDIR)/$(PRJNAME)
TEST_MAIN = ../test/test.cpp
TEST_TARGET = $(BLDDIR)/$(PRJNAME)-test

CXX = g++
CXXFLAGS = -std=c++23 -fno-rtti -O3 $(addprefix -I, $(INCLUDEDIRS))
CXXFLAGS += -march=native
CXXFLAGS += -Wall -Wextra -Wpedantic -Wfatal-errors -Werror
CXXFLAGS += -Wshadow -Wconversion -Wsign-conversion
CXXFLAGS += -Wsign-promo -Wstrict-overflow=2 -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Wredundant-decls -Wstrict-null-sentinel -Wundef
CXXFLAGS += -I/usr/local/include -L/usr/local/lib
CXX_VER := $(shell $(CXX) -dumpversion)

#MSVC = cl.exe
#MSVCFLAGS = -utf-8 -std:c++latest -permissive- -Zc:wchar_t -Zc:forScope -Zc:inline -Gd -fp:precise -EHsc -TP
#MSVCFLAGS += -MD -Gm- -GS -Gy -O2 -Oi -nologo
#MSVCFLAGS += -W4 -WX -external:W3 -FC -diagnostics:column -sdl
#MSVCFLAGS += $(addprefix -I, $(INCLUDEDIRS)) -ID:\HD\dev\vcpkg\installed\x64-windows\include
#MSVCFLAGS += -D X64 -D NOMINMAX -D NDEBUG -D _CONSOLE -D WIN32_LEAN_AND_MEAN -D VC_EXTRALEAN
#MSVCFLAGS = -c -Zs # Just syntax

default: executable

all: executable test

analisys: CXXFLAGS += --coverage
test: CXXFLAGS += -D_FORTIFY_SOURCE=3 -fsanitize=address -fsanitize=undefined -fsanitize=leak -fsanitize=pointer-subtract -fsanitize=pointer-compare -fno-omit-frame-pointer -fstack-protector-all -fstack-clash-protection -fcf-protection
#test: CXXFLAGS += -fanalyzer
debug: CXXFLAGS += -DDEBUG -D_DEBUG -g
debug: executable test

executable: $(CPPS) $(HEADERS) makefile
	$(info [$(TARGET), compiler ver $(CXX_VER)])
	@mkdir -p ${BLDDIR}
	$(CXX) -o $(TARGET) $(CXXFLAGS) $(CPPS)

test: $(TEST_MAIN) $(HEADERS) makefile
	$(info [$(TEST_TARGET), compiler ver $(CXX_VER)])
	@mkdir -p ${BLDDIR}
	$(CXX) -o $(TEST_TARGET) $(CXXFLAGS) $(TEST_MAIN)

clean:
	$(info [clean])
	#rm $(BLDDIR)/*.o
	rm $(TARGET)
	rm $(TEST_TARGET)
