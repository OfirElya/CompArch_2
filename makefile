
#cacheSim: cacheSim.cpp
#	g++ -o cacheSim cacheSim.cpp

myTest: myTest.cpp utils.cpp utils.h
	g++ -o myTest myTest.cpp utils.cpp

.PHONY: clean
clean:
	rm -f *.o
	rm -f myTest
