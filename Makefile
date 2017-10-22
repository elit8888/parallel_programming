CXX = gcc
CXXFLAGS = -std=gnu99 -pthread -O2 -s

pi: pi.c
	$(CXX) $(CXXFLAGS) pi.c -o pi

debug: pi.c
	$(CXX) $(CXXFLAGS) -D DEBUG pi.c -o pi

clean:
	rm -f pi
