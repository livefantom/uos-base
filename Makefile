####### Compiler, tools and options

CC       = gcc
CXX      = g++
CFLAGS   = -pipe -Wall -W -march=i686 -mtune=i686 -g -DGLX_GLXEXT_LEGACY -fno-use-cxa-atexit -fexceptions -DLINUX
CXXFLAGS = -pipe -Wall -W -march=i686 -mtune=i686 -g -DGLX_GLXEXT_LEGACY -fno-use-cxa-atexit -fexceptions -DLINUX
INCPATH  = -I. -I$(ESUITE_HOME)/includes
LINK     = g++
LFLAGS   =
LIBS     = -L$(ESUITE_HOME)/lib -lEBase -pthread
AR       = ar cqs
RANLIB   = 
TAR      = tar -cf
GZIP     = gzip -9f
COPY     = cp -f
COPY_FILE= $(COPY) -p
COPY_DIR = $(COPY) -pR
DEL_FILE = rm -f
SYMLINK  = ln -sf
DEL_DIR  = rmdir
MOVE     = mv -f
CHK_DIR_EXISTS= test -d
MKDIR    = mkdir -p

####### Output directory

OBJECTS_DIR = 

####### Files

HEADERS = 
SOURCES = pfauth.cpp \
		  socket.cpp \
		  mutex.cpp \
		  logger.cpp \
		  thread.cpp \
		  sockwatcher.cpp \
		  connection.cpp \
		  connector.cpp \
		  authmsg.cpp \
		  main.cpp
OBJECTS = pfauth.o \
		  socket.o \
		  mutex.o \
		  logger.o \
		  thread.o \
		  sockwatcher.o \
		  connection.o \
		  connector.o \
		  authmsg.o \
		  main.o

DESTDIR  = 
TARGET   = pfauth

first: all
####### Implicit rules

.SUFFIXES: .c .cpp .cc .cxx .C

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cc.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.C.o:
	$(CXX) -c $(CXXFLAGS) $(INCPATH) -o $@ $<

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o $@ $<

####### Build rules

all: Makefile $(TARGET) 

$(TARGET): $(OBJECTS) 
	$(LINK) $(LFLAGS) -o $@ $^ $(LIBS)

clean:
	-$(DEL_FILE) $(TARGET) $(OBJECTS) 
	-$(DEL_FILE) *~ core *.core


####### Install

install: all 

uninstall: 
