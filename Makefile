#  Makefile for setd and mark utilities (C++ version)
#  Based on original Makefile by Sunil William Savkar
#  Modernized for C++ compilation
#
#  *  Must change the BINDIR, MANDIR to point to the
#     appropriate areas
#  *  Must change MACHINE_TYPE to reflect the type of machine
#     you are running on (i.e.  HP, SUN, RS6000, etcetera)
#

DESTDIR= $(HOME)
BINDIR= $(DESTDIR)/bin/$$ARCH
MANDIR = $(DESTDIR)/man/man1
#MACHINE_TYPE = $$ARCH

TARGET1 = setd$(EXT)
TARGET2 = mark$(EXT)

CXX	= g++
OFLAGS	= -O2 -std=c++14
CFLAGS	= $(OFLAGS) 
LDFLAGS =
# Windows support
ifeq ($(OS),Windows_NT)
    CXX = g++
    EXT = .exe
else
    EXT =
endif
MAN1 = setd.1
MAN2 = mark.1
SOURCES1 = setd.cpp
SOURCES2 = mark.cpp
SOURCES3 = mark_db.cpp
OBJECTS1 = setd.o
OBJECTS2 = mark.o
OBJECTS3 = mark_db.o
HEADERS1 = setd.hpp
HEADERS2 = mark_db.hpp

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(OBJECTS1) $(OBJECTS3)
	$(CXX) $(LDFLAGS) $(OBJECTS1) $(OBJECTS3) -o $(TARGET1)

$(TARGET2): $(OBJECTS2) $(OBJECTS3)
	$(CXX) $(LDFLAGS) $(OBJECTS2) $(OBJECTS3) -o $(TARGET2)

setd.o: $(HEADERS1) $(SOURCES1)
	$(CXX) $(CFLAGS) -c $(SOURCES1) -o $(OBJECTS1)

mark.o: $(HEADERS2) $(SOURCES2)
	$(CXX) $(CFLAGS) -c $(SOURCES2) -o $(OBJECTS2)

mark_db.o: $(HEADERS2) $(SOURCES3)
	$(CXX) $(CFLAGS) -c $(SOURCES3) -o $(OBJECTS3)

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

# Test targets
.PHONY: test test-all test-bash test-zsh test-csh test-tcsh test-sh test-dash test-ksh test-fish
.PHONY: test-build test-clean

# Run all tests
test: test-all

test-all:
	@echo "Running all tests..."
	@cd tests && ./run_tests.sh

# Test specific shell
test-bash:
	@cd tests && ./run_tests.sh -s bash

test-zsh:
	@cd tests && ./run_tests.sh -s zsh

test-csh:
	@cd tests && ./run_tests.sh -s csh

test-tcsh:
	@cd tests && ./run_tests.sh -s tcsh

test-sh:
	@cd tests && ./run_tests.sh -s sh

test-dash:
	@cd tests && ./run_tests.sh -s dash

test-ksh:
	@cd tests && ./run_tests.sh -s ksh

test-fish:
	@cd tests && ./run_tests.sh -s fish

# Build only
test-build:
	@cd tests && ./run_tests.sh --build-only

# Clean test artifacts
test-clean:
	@echo "Cleaning test artifacts..."
	@rm -rf /tmp/test_tree 2>/dev/null || true
	@rm -rf $$HOME/bin/setd $$HOME/bin/mark 2>/dev/null || true
