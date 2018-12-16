SHELL = /bin/sh
# définition des commandes utilisées
CC = g++
ECHO = echo
RM = rm -f
TAR = tar
MKDIR = mkdir
CHMOD = chmod
CP = rsync -R
# déclaration des options du compilateur
PG_FLAGS =
SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LDFLAGS = $(shell sdl2-config --libs)
CPPFLAGS = -I. -I/Users/amsi/local/include $(SDL_CFLAGS)
CFLAGS = -Wall -O3
LDFLAGS = -lm -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -lopencv_objdetect -lopencv_core -lopencv_videoio -L/Users/amsi/local/lib -lGL4Dummies $(SDL_LDFLAGS) -lSDL2_image

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
	MACOSX_DEPLOYMENT_TARGET = 10.9
        CFLAGS += -I/usr/X11R6/include -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET) -I/opt/local/include/ -I/opt/local/include/opencv2 -Wno-unused-function
        LDFLAGS += -lopencv_videoio -framework OpenGL -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET) 
else
        LDFLAGS +=  -L/usr/lib -L/usr/X11R6/lib -lGL -lGLU
endif

#définition des fichiers et dossiers
PROGNAME = GLSLExample
PACKAGE=$(PROGNAME)
VERSION = 01.3
distdir = $(PACKAGE)-$(VERSION)
HEADERS = 
SOURCES = window.cpp 
OBJ = $(SOURCES:.cpp=.o)
DOXYFILE = documentation/Doxyfile
EXTRAFILES = haarcascade_eye.xml haarcascade_frontalface_default.xml COPYING shaders/basic.vs shaders/basic.fs
DISTFILES = $(SOURCES) Makefile $(HEADERS) $(DOXYFILE) $(EXTRAFILES)

all: $(PROGNAME)

$(PROGNAME): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(PROGNAME)

%.o: %.cpp
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

dist: distdir
	$(CHMOD) -R a+r $(distdir)
	$(TAR) zcvf $(distdir).tgz $(distdir)
	$(RM) -r $(distdir)

distdir: $(DISTFILES)
	$(RM) -r $(distdir)
	$(MKDIR) $(distdir)
	$(CHMOD) 777 $(distdir)
	$(CP) $(DISTFILES) $(distdir)

doc: $(DOXYFILE)
	cat $< | sed -e "s/PROJECT_NAME *=.*/PROJECT_NAME = $(PROGNAME)/" | sed -e "s/PROJECT_NUMBER *=.*/PROJECT_NUMBER = $(VERSION)/" >> $<.new
	mv -f $<.new $<
	cd documentation && doxygen && cd ..

clean:
	@$(RM) -r $(PROGNAME) $(OBJ) *~ $(distdir).tgz gmon.out core.* documentation/*~ shaders/*~ documentation/html
