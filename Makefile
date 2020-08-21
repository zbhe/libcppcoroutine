all: coroutine
coroutine: cppco.cpp
	g++ -O2 -o coroutine cppco.cpp
