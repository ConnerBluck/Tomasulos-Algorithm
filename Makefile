
CXX = g++
SRC = procsim.cpp procsim_driver.cpp
INCLUDE = procsim.h
CXXFLAGS := -Wall -Wextra -Wconversion -std=c++11 # -Werror
DEBUG := -g -O0
#RELEASE := -O3 # Feel free to enable to help you test

build:
	$(CXX) $(RELEASE) $(CXXFLAGS) $(SRC) -o procsim

newdebug:
	$(CXX) $(DEBUG) $(CXXFLAGS) $(SRC) -o procsim

clean:
	rm -f procsim *.o

submit:
	tar -cvzf project3-submit.tar.gz procsim.cpp procsim_driver.cpp procsim.hpp Makefile
