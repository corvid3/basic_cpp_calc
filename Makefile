default:
	clang++ main.cc -O2 -o calc -Wall -Wextra -Werror --std=c++17
	strip -s calc
