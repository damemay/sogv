CC = clang

SOURCES = src/glad.c \
	  src/sogv_base.c \
	  src/sogv_tri.c

FLAGS = -c \
	-fpic \
	-g

PACK_LIB = ar rcs libsogv.so *.o

all :
	$(CC) -Iinclude $(SOURCES) $(FLAGS) && mv *.o lib/ && cd lib && $(PACK_LIB)

clean :
	cd lib && rm -rf *
