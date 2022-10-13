EXTRA_CFLAGS =-DHAVE_NASM -DHAVE_SDL -DHAVE_PNG -DHAVE_JPEG
INCLUDE	= -Iinclude -I/usr/local/include/freetype2 -I/usr/include/freetype2
CFLAGS	= -fPIC -ggdb -Wall  $(INCLUDE) $(EXTRA_CFLAGS) `ppl6-config --cflags debug`
LIB		= -fPIC -L/usr/local/lib   -lpthread `ppl6-config --libs debug`  -lstdc++
PROGRAM	= pplfontmaker

LIBDEP	:= $(shell ppl6-config --ppllib debug)

OBJECTS=compile/main.o compile/common.o \
	compile/font5maker.o \
	compile/font6maker.o
	

	
$(PROGRAM): compile $(OBJECTS) $(LIBDEP)
	$(CXX) -o $(PROGRAM) $(OBJECTS) $(CFLAGS) $(LIB)
	-chmod 755 $(PROGRAM)

all: $(PROGRAM)

compile:
	-mkdir -p compile

clean:
	-rm -f compile/*.o $(PROGRAM) *.core
	
install: $(PROGRAM)
	-cp $(PROGRAM) /usr/local/bin

	
compile/main.o: src/main.cpp include/fontmaker.h Makefile
	$(CXX) -o compile/main.o -c src/main.cpp $(CFLAGS)

compile/common.o: src/common.cpp include/fontmaker.h Makefile
	$(CXX) -o compile/common.o -c src/common.cpp $(CFLAGS)
	
compile/font5maker.o: src/font5maker.cpp include/fontmaker.h Makefile
	$(CXX) -o compile/font5maker.o -c src/font5maker.cpp $(CFLAGS)

compile/font6maker.o: src/font6maker.cpp include/fontmaker.h Makefile
	$(CXX) -o compile/font6maker.o -c src/font6maker.cpp $(CFLAGS)
	
