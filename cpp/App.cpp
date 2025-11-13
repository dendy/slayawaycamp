
#include "App.hpp"

#include <charconv>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

#include "Player.hpp"
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
		_execMoobaa();
	}

	if (!map_.shortName.empty()) {
		Map::draw(map_);
		_execMap();
	}
}


void App::_execMap() noexcept
{
	Solver solver(map_, [] (Solver::Solution &&) {});
}


void App::_execMoobaa() noexcept
{
	struct Movie {
		int id;
		std::filesystem::path dirname;
		std::string shortName;
		std::string fullName;

		bool operator<(const Movie & other) const noexcept { return id < other.id; }
	};

	struct Serie {
		int id;
		std::filesystem::path filename;

		bool operator<(const Serie & other) const noexcept { return id < other.id; }
	};

	const auto movies = [] () -> std::vector<Movie> {
		std::vector<Movie> movies;
		for (const std::filesystem::directory_entry & entry :
				std::filesystem::directory_iterator(SLAYAWAYCAMP_MOVIES_DIR)) {
			if (!entry.is_directory()) continue;
			Movie movie {
				.id = [&entry] () -> int {
					const std::string name = entry.path().filename().string();
					const std::string_view s = std::string_view(name);
					int id = -1;
					const std::from_chars_result r = std::from_chars(
							s.data() + 1, s.data() + 3, id);
					if (std::make_error_condition(r.ec) || id <= 0) {
						fprintf(stderr, "Not a movie: %s\n", entry.path().c_str());
						return -1;
					}
					return id;
				}(),
			};
			if (movie.id == -1) continue;
			if (std::find_if(movies.begin(), movies.end(),
					[&movie] (const Movie & m) { return m.id == movie.id; }) != movies.end()) {
				fprintf(stderr, "Duplicated movie id: %d\n", movie.id);
				continue;
			}
			std::ifstream file(entry.path() / "movie.info");
			if (!file.good()) continue;
			int number = 0;
			std::string line;
			while (std::getline(file, line)) {
				number++;
				switch (number) {
				case 1:
					movie.shortName = std::move(line);
					break;
				case 2:
					movie.fullName = std::move(line);
					break;
				default:
					fprintf(stderr, "Invalid movie.info: %s\n", entry.path().c_str());
					continue;
				}
			}
			movie.dirname = entry.path().filename();
			movies.push_back(std::move(movie));
		}
		std::sort(movies.begin(), movies.end());
		return movies;
	}();

	const auto testCategory = [this] (const Movie & movie, const Category category) {
		const auto & moobaaMovie = [this, &movie] () -> const Moobaa::Movie & {
			auto it = moobaa_.movieForName.find(movie.shortName);
			if (it == moobaa_.movieForName.end()) {
				it = moobaa_.movieForName.find(movie.shortName + ": " + movie.fullName);
			}
			if (it == moobaa_.movieForName.end()) {
				throw std::runtime_error("");
			}
			return it->second;
		}();

		printf("\n");
		printf("moobaa: Testing movie category: (%02d) %s - %s\n",
				movie.id, movie.shortName.c_str(), nameForCategory(category).data());
		fflush(stdout);

		const auto & moobaaSeries = [&moobaaMovie, category] () -> const Moobaa::Series & {
			switch (category) {
			case Category::Base: return moobaaMovie.baseSeries;
			case Category::NC17: return moobaaMovie.nc17Series;
			case Category::Xtra: return moobaaMovie.xtraSeries;
			}
			assert(false);
		}();

		const std::filesystem::path seriesPath =
				std::filesystem::path(SLAYAWAYCAMP_MOVIES_DIR) / movie.dirname / nameForCategory(category);

		const auto series = [&seriesPath] () -> std::vector<Serie> {
			std::vector<Serie> series;
			for (const std::filesystem::directory_entry & entry :
					std::filesystem::directory_iterator(seriesPath)) {
				if (!entry.is_regular_file()) continue;
				if (entry.path().extension() != kSerieExtension) continue;
				Serie serie {
					.id = [&entry] () -> int {
						const std::string name = entry.path().filename().string();
						const std::string_view s = std::string_view(name);
						int id = -1;
						const std::from_chars_result r = std::from_chars(
								s.data() + 10, s.data() + 12, id);
						if (std::make_error_condition(r.ec) || id <= 0) {
							fprintf(stderr, "Not a serie: %s\n", entry.path().c_str());
							return -1;
						}
						return id;
					}(),
				};
				serie.filename = entry.path().filename();
				series.push_back(std::move(serie));
			}
			std::sort(series.begin(), series.end());
			return series;
		}();

		for (const Serie & serie : series) {
			printf("\n");
			printf("moobaa:     serie: (%02d) %s\n", serie.id, serie.filename.c_str());
			fflush(stdout);

			const int moobaaIndex = serie.id - 1;
			if (moobaaIndex >= moobaaSeries.size()) {
				printf("moobaa:         fail: no mooba steps\n");
				continue;
			}

			const Moobaa::Serie & moobaaSerie = moobaaSeries[moobaaIndex];
			printf("moobaa:         moobaa name: %s (%s)\n",
					moobaaSerie.shortName.c_str(), moobaaSerie.fullName.c_str());

			const Map map = Map::load(seriesPath / serie.filename);

			const auto bestSolution = [&map] () -> Solver::Solution {
				Solver::Solution best;
				Solver solver(map, [&best] (Solver::Solution && solution) {
					if (best.steps.empty() || solution.steps.size() < best.steps.size()) {
						best = std::move(solution);
					}
				});
				assert(!best.steps.empty());
				return best;
			}();

			const auto checkName = [&moobaaSerie, &map] () -> bool {
				std::string mname = moobaaSerie.shortName;
				std::for_each(mname.begin(), mname.end(), [] (char & c) { c = std::tolower(c); });
				return mname == map.shortName;
			};
			if (!checkName()) {
				printf("moobaa:         fail: name mismatch\n");
				continue;
			}

			printf("moobaa:         expected steps (%d): %s\n",
					int(moobaaSerie.steps.size()), stepsToString(moobaaSerie.steps).c_str());

			State state = map.state;

			Player::Result result;
			int stepCount = 0;
			for (const Dir dir : moobaaSerie.steps) {
				stepCount++;
				result = Player::play(map, state, dir);
				if (stepCount < moobaaSerie.steps.size() && result != Player::Result::None) {
					printf("moobaa:         fail: unexpected end (%s) after steps: (%d)\n",
							result == Player::Result::Win ? "win" : "fail", stepCount);
					break;
				}
			}
			if (stepCount != moobaaSerie.steps.size()) {
				continue;
			}

			if (result != Player::Result::Win) {
				printf("moobaa:         fail: not a win (%s)\n",
						result == Player::Result::None ? "none" : "fail");
				continue;
			}

			assert(bestSolution.steps.size() <= moobaaSerie.steps.size());

			printf("moobaa:         OK\n");

			if (bestSolution.steps.size() < moobaaSerie.steps.size()) {
				printf("moobaa:         FOUND BETTER: (%d) [%s] -> (%d) [%s]\n",
						int(moobaaSerie.steps.size()),
						stepsToString(moobaaSerie.steps).c_str(),
						int(bestSolution.steps.size()),
						stepsToString(bestSolution.steps).c_str());
			}
		}
	};

	for (const Movie & movie : movies) {
		for (const Category & category : kAllCategories) {
			try {
				testCategory(movie, category);
			} catch (const std::exception &) {
			}
		}
	}
}
