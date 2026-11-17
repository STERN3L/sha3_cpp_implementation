CXX = c++
CXXFLAGS = -O2 -Wall -Wextra -std=c++11

sha3: sha3.cpp sha3.h
	$(CXX) $(CXXFLAGS) -o sha3 sha3.cpp

test: sha3
	sh test.sh ./sha3

clean:
	rm -f sha3
	rm -rf tests_tmp

dist: clean
	tar czf sha3.cpp sha3.h Makefile test.sh
