
BUILDDIR = ./build
BINDIR   = $(BUILDDIR)/bin
INCDIR   = $(BUILDDIR)/include
LIBDIR   = $(BUILDDIR)/lib
OBJDIR   = $(BUILDDIR)/obj
LIBPATH  = $(LIBDIR)/libummapio.so
UMAPFLAG = -lummapio
CXX			 = g++
CXXFLAGS   = -O2 -I. -L$(LIBDIR) -L. 
CXXFLAGS  += -Wall -Wno-unused-label -Wno-unused-function
#CXXFLAGS  += -DDEBUG_PRINT=$(or $(DEBUG_PRINT),1)
LDFLAGS  = -pthread -lrt -lparallax 

all: setup $(LIBPATH) $(BINDIR)/test

setup:
	@mkdir -p $(BINDIR) $(INCDIR) $(LIBDIR) $(OBJDIR) 2> /dev/null
	@cp ./include/ummap.h $(INCDIR) 2> /dev/null

$(BINDIR)/test: $(LIBPATH) ./test/main.c
	$(CXX) $(CXXFLAGS) ./test/main.c -o $(BINDIR)/test $(UMAPFLAG)

$(LIBPATH): $(OBJDIR)/ummap.o $(OBJDIR)/futex.o $(OBJDIR)/parallax_db.o
	$(CXX) $(OBJDIR)/*.o -shared -fPIC -o $(LIBPATH) $(LDFLAGS)

$(OBJDIR)/ummap.o: ./src/ummap.cpp ./include/ummap.h ./include/common.h ./include/ummap_types.h 
	$(CXX) $(CXXFLAGS) -c -fPIC ./src/ummap.cpp -o $(OBJDIR)/ummap.o

$(OBJDIR)/parallax_db.o: ./src/parallax_db.cpp ./include/parallax_db.hpp
	$(CXX) $(CXXFLAGS) -c -fPIC ./src/parallax_db.cpp -o $(OBJDIR)/parallax_db.o

$(OBJDIR)/futex.o: ./src/futex.c ./include/futex.h
	$(CXX) $(CXXFLAGS) -c -fPIC ./src/futex.c -o $(OBJDIR)/futex.o

clean:
	@rm -rf $(BUILDDIR)

rebuild: clean all
