
#include "App.hpp"

#include <cstring>

#include "Solver.hpp"




App::App(Args && args) :
	moobaa_([&args] () -> Moobaa {
		if (args.moobaa) {
			return Moobaa::load();
		} else {
			return {};
		}
	}()),
	map_([&args] () -> Map {
		if (!args.mapFilePath.empty()) {
			return Map::load(args.mapFilePath);
		} else {
			return {};
		}
	}())
{
	if (args.moobaa) {
		for (const auto & it : moobaa_.movieForName) {
			const Moobaa::Movie & movie = it.second;
			printf("movie: %s\n", movie.name.c_str());
		}
	}

	if (!map_.shortName.empty()) {
		Map::draw(map_);
		exec();
	}
}


void App::exec() noexcept
{
	Solver solver(map_);
}
