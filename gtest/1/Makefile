all:main
main:
	g++ -o functions.o -c functions.cpp
	g++ -o functionsTest.o -c functionsTest.cpp -I./include
	g++ -o TestAll.o -c TestAll.cpp -I./include
	g++ -o main *.o -I./include -L./lib -lgtest -lpthread
clean:
	rm -rf *.o main

