run: memlab.cpp demo1.cpp memlab.hpp demo2.cpp
	g++ memlab.cpp demo1.cpp -pthread -o demo1
	g++ memlab.cpp demo2.cpp -pthread -o demo2
	g++ memlab.cpp demo3.cpp -pthread -o demo3


run1: demo1
	./demo1 2 > out1.txt

run2: demo2
	./demo2 2 > out2.txt

run3: demo3
	./demo3 2 > out3.txt

clean: 
	rm demo1 demo2 demo3 out1.txt out2.txt out3.txt
