#include <iostream>

class A {
public:
	A(char * c) {
		std::cout << "Create A with: " << c << std::endl;
	}
	~A() {
		std::cout << "Destruct A" << std::endl;
	}
};

int main(int argc, char ** argv) {
	for (auto i = 0; i < argc; ++i) {
		A a(argv[i]);
	}
}
