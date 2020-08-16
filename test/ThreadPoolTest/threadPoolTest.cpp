#include "ThreadPool.h"

#include <iostream>
#include <chrono>


void threadFunc(int n) {
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	printf("%d\n", n);
}


int main() {
	ThreadPool pool(4);
	for(int i = 0; i < 10; i++) {
		pool.addTask(std::bind(&threadFunc, i));
	}
	return 0;
}
