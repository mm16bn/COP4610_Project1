CXX = clang++
CXXFLAGS = -pedantic -std=c++11

parser: parser.o
	 $(CXX) $(CXXFLAGS) -o parser parser.o

parser.o: parser.c
	$(CXX) $(CXXFLAGS) -c parser.c
