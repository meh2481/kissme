OBJECTS = main.o signalhandler.o sound.o
LIBS = -L./dep/lib -lttvfs -ltag -ltyrsound -lopenal -logg -lvorbis -lvorbisfile -lopus
HEADER = -I./dep/include

GTKINCLUDE = `pkg-config gtk+-3.0 gmodule-2.0 --cflags`
GTKLIB = `pkg-config gtk+-3.0 gmodule-2.0 --libs`

all : kissme

kissme : $(OBJECTS)
	g++ -Wall -ggdb -g -O2 -o $@ $^ $(LIBS) $(GTKLIB)

%.o: %.cpp
	g++ -c -MMD -ggdb -g -o $@ $< $(HEADER) $(GTKINCLUDE)

-include $(OBJECTS:.o=.d)

clean:
	rm -rf *.o *.d kissme
