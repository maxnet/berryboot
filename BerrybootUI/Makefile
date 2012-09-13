LDFLAGS +=-lncurses -ldialog

all: BerrybootUI

BerrybootUI: main.o mainwindow.o berryboot.o
	$(CXX) -o $@ $^ $(CFLAGS) $(LDFLAGS)
  
