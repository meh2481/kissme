objects = main.o examplewindow.o sdlsound.o
libs = -L./lib -lttvfs -lopenal -lSDL -lSDL_soundn -lmikmod -lvorbisfile -lspeex
HEADER = -I./include

GTKINCLUDE = `pkg-config gtkmm-3.0 --cflags`
GTKLIB = `pkg-config gtkmm-3.0 --libs`

all : kissme

kissme : $(objects)
	g++ -Wall -O2 -o $@ $^ $(libs) $(GTKLIB)

%.o: %.cpp
	g++ -c -MMD -o $@ $< $(HEADER) $(GTKINCLUDE)

-include $(objects:.o=.d)

clean:
	rm -rf *.o *.d kissme
