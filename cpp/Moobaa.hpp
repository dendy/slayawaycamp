
#pragma once

#include <unordered_map>
#include <vector>

#include "Common.hpp"




struct Moobaa {
	struct Serie {
		std::string shortName;
		std::string fullName;
		Steps steps;
	};

	using Series = std::vector<Serie>;

	struct Movie {
		std::string name;
		Series baseSeries;
		Series nc17Series;
		Series xtraSeries;
	};

	std::unordered_map<std::string, Movie> movieForName;

	static Moobaa load();
};
