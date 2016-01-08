CXX=clang++
CPPFLAGS=-pthread -I/usr/include/OGRE `pkg-config OIS --cflags` `pkg-config bullet --cflags` -g
LIBS=`pkg-config OGRE --libs` `pkg-config OIS --libs` `pkg-config bullet --libs` -lboost_system

test: main.cpp BtOgre.cpp
	$(CXX) main.cpp BtOgre.cpp -o test $(CPPFLAGS) $(LIBS)

clean:
	rm test
