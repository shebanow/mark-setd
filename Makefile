#  Makefile for setd and mark utilities (C++ version)
#  Based on original Makefile by Sunil William Savkar
#  Modernized for C++ compilation
#
#  *  Must change the BINDIR, MANDIR to point to the
#     appropriate areas
#  *  Must change MACHINE_TYPE to reflect the type of machine
#     you are running on (i.e.  HP, SUN, RS6000, etcetera)
#

DESTDIR= /home/mshebanow/
BINDIR= $(DESTDIR)/bin/$$ARCH
MANDIR = $(DESTDIR)/man/man1
#MACHINE_TYPE = $$ARCH

TARGET1 = setd
TARGET2 = mark

CXX	= g++
OFLAGS	= -O2 -std=c++14
CFLAGS	= $(OFLAGS) 
LDFLAGS =
MAN1 = setd.1
MAN2 = mark.1
SOURCES1 = setd.cpp
SOURCES2 = mark.cpp
OBJECTS1 = setd.o
OBJECTS2 = mark.o
HEADERS1 = setd.hpp
HEADERS2 = mark.hpp

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJECTS1)
	$(CXX) $(LDFLAGS) $(OBJECTS1) -o $(TARGET1)

$(TARGET2): $(OBJECTS2)
	$(CXX) $(LDFLAGS) $(OBJECTS2) -o $(TARGET2)

setd.o: $(HEADERS1) $(SOURCES1)
	$(CXX) $(CFLAGS) -c $(SOURCES1) -o $(OBJECTS1)

mark.o: $(HEADERS2) $(SOURCES2)
	$(CXX) $(CFLAGS) -c $(SOURCES2) -o $(OBJECTS2)

clean	:
		rm -f *.o

clobber :	clean
		rm -f $(TARGET1)
		rm -f $(TARGET2)

installman: setd.1 mark.1
	cp $(MAN1) $(MANDIR)/$(MAN1)
	cp $(MAN2) $(MANDIR)/$(MAN2)

installexec: all
		cp $(TARGET1) $(BINDIR)/$(TARGET1)
		cp $(TARGET2) $(BINDIR)/$(TARGET2)

install :	all installman installexec

tar:
	rm -fr mark-setd.src
	mkdir mark-setd.src
	cp *.cpp mark-setd.src
	cp *.hpp mark-setd.src
	cp *.h mark-setd.src 2>/dev/null || true
	cp setd.1 mark-setd.src
	cp mark.1 mark-setd.src
	cp README.md mark-setd.src
	cp LICENSE mark-setd.src
	cp Makefile mark-setd.src
	cp SETD_BASH mark-setd.src
	cp SETD_CSHRC mark-setd.src
	tar -cf - mark-setd.src | compress > mark-setd.tar.Z
	rm -fr mark-setd.src
