#include <cstdlib>

int main( int argc, char* argv[] ) {
	size_t size = sizeof(void*);
	if ( 4 == size ) {
		return 0;
	}
	return 1;
}