objects = main.o
libs = -L./lib -lSDL -lSDL_mixer
HEADER = -I./include

all : kissme

kissme : $(objects)
	g++ -Wall -O2 -o $@ $^ $(libs)

%.o: %.cpp
	g++ -c -MMD -o $@ $< $(HEADER)

-include $(objects:.o=.d)

clean:
	rm -rf *.o *.d kissme
