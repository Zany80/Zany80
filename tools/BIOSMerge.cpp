#include <iostream>
#include <fstream>


/**
 * Extremely simple program
 * Just merges three files, ensuring each has an exact length of 0x4000
 */

int main(int argc, const char **argv) {
	std::ofstream out(argv[1]);
	for (int i = 2; i < 5; i++) {
		std::cerr << "Attempting opening file "<<i<<": \""<<argv[i]<<"\"\n";
		std::ifstream in(argv[i], std::ios::ate | std::ios::binary);
		if (in.is_open()) {
			char buffer[0x4000]{0};
			int size = in.tellg();
			in.seekg(0);
			if (size > 0x4000) {
				std::cerr << "Error 2: file too big! File "<<i<<": \""<<argv[i]<<"\"\n";
				return 2;
			}
			in.read(buffer, size);
			in.close();
			out.write(buffer, 0x4000);
		}
		else {
			std::cerr << "Error 1: unable to open file "<<i<<": \""<<argv[i]<<"\"\n";
			return 1;
		}
	}
	return 0;
}
