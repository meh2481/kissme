OBJECTS = main.o signalhandler.o sound.o
LIBS = -L./lib -lttvfs -lopenal -lSDL -lSDL_mixer
HEADER = -I./include

GTKINCLUDE = `pkg-config gtk+-3.0 gmodule-2.0 --cflags`
GTKLIB = `pkg-config gtk+-3.0 gmodule-2.0 --libs`

all : kissme

kissme : $(OBJECTS)
	g++ -Wall -O2 -o $@ $^ $(LIBS) $(GTKLIB)

%.o: %.cpp
	g++ -c -MMD -o $@ $< $(HEADER) $(GTKINCLUDE)

-include $(OBJECTS:.o=.d)

clean:
	rm -rf *.o *.d kissme
