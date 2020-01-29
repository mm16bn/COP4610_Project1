CXX = clang++
CXXFLAGS = -pedantic -std=c++11

shell: parser.o
	 $(CXX) $(CXXFLAGS) -o shell shell.o

shell.o: parser.c
	$(CXX) $(CXXFLAGS) -c parser.c
