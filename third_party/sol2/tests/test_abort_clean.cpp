#include <cstdlib>

struct pre_main {
	pre_main() {
#ifdef SOL2_CI
#ifdef _MSC_VER
	_set_abort_behavior(0, _WRITE_ABORT_MSG);
#endif
#endif
	}
} pm;
