
#include <stdexcept>

#include "App.hpp"




int main(int argc, char ** argv)
{
	if (argc != 2) {
		throw std::runtime_error("Usage: slayawaycamp <level>");
	}

	App app(argv[1]);

	return 0;
}
