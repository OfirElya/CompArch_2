
#cacheSim: cacheSim.cpp
#	g++ -o cacheSim cacheSim.cpp

myTest: myTest.cpp utils.cpp utils.h
	g++ -Wall -std=c++11 -o cacheSim cacheSim.cpp utils.cpp

.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim
