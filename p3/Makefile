CXX = gcc
CXXFLAGS = -lm

prime: prime/prime.c
	$(CXX) $(CXXFLAGS) $< -o $@

integrate: integrate/integrate.c
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f prime integrate
