
#include "Moobaa.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string_view>

#include "Common.hpp"




Moobaa Moobaa::load()
{
	const std::filesystem::path filePath = std::filesystem::path(SLAYAWAYCAMP_REFERENCE_DIR) / "moobaa.txt";

	static constexpr std::string_view kBaseCategory = "NORMAL SCENES";
	static constexpr std::string_view kNc17Category = "NC-17 SCENES";
	static constexpr std::string_view kXtraCategory = "DELETED SCENES";

	struct MovieSuffix {
		std::string_view suffix;
		std::string_view category;
	};
	const auto categoryForSuffix = [] (const std::string_view & name) -> MovieSuffix {
		static constexpr std::string_view kNc17Suffix = " (NC-17 Scenes)";
		static constexpr std::string_view kXtraSuffix = " (Deleted Scenes)";
		if (name.ends_with(kNc17Suffix)) {
			return MovieSuffix {
				.suffix = kNc17Suffix,
				.category = kNc17Category,
			};
		}
		if (name.ends_with(kXtraSuffix)) {
			return MovieSuffix {
				.suffix = kXtraSuffix,
				.category = kXtraCategory,
			};
		}
		return MovieSuffix {
			.suffix = {},
			.category = kBaseCategory,
		};
	};

	std::ifstream file(filePath);
	assert(file.good());

	enum class State {
		Category,
		MovieName,
		SerieDelimiter,
		SerieName,
		SerieSteps,
		Next,
	};

	std::unordered_map<std::string, Movie> movieForName;
	std::string_view category;
	Movie * movie = nullptr;
	Serie serie;
	std::string title;
	State state = State::Category;

	const auto tryCategory = [&category] (const std::string_view & s) -> bool {
		for (const std::string_view c : {kBaseCategory, kNc17Category, kXtraCategory}) {
			if (s == c) {
				category = c;
				return true;
			}
		}
		return false;
	};

	const auto getSeries = [&movie, &category] () -> Series & {
		if (category == kBaseCategory) return movie->baseSeries;
		if (category == kNc17Category) return movie->nc17Series;
		if (category == kXtraCategory) return movie->xtraSeries;
		assert(false);
	};

	const auto splitSerieName = [&serie, &title] (const std::string_view & s) {
		title = s;
		const std::string_view::size_type slashPos = s.find(" / ");
		if (slashPos == std::string_view::npos) {
			serie.shortName = s;
		} else {
			serie.shortName = s.substr(0, slashPos);
			serie.fullName = s.substr(slashPos + 3);
		}
	};

	const auto completeSerie = [&serie, &getSeries] {
		if (!serie.shortName.empty()) {
			assert(!serie.steps.empty());
			getSeries().push_back(std::move(serie));
		}
	};

	const auto completeMovie = [&movie, &movieForName, &completeSerie] {
		if (!movie) return;
		completeSerie();
		movie = nullptr;
	};

	const auto addMovie = [&movieForName, &completeMovie, &movie, &categoryForSuffix, &category]
			(std::string && name) {
		assert(!name.empty());
		const MovieSuffix suffix = categoryForSuffix(name);
// printf("am: name=(%s) s=%s c=%s\n", name.c_str(), suffix.suffix.data(), suffix.category.data()); fflush(stdout);
		assert(suffix.category == category);
		std::string realName = name.substr(0, name.size() - suffix.suffix.size());
		completeMovie();
		auto r = movieForName.insert({realName, Movie {
			.name = realName,
		}});
		auto & it = r.first;
		movie = &it->second;
	};

	const auto next = [&tryCategory, &completeMovie, &addMovie, &state] (std::string && line) {
		if (line.empty()) return;

		completeMovie();

		if (tryCategory(line)) {
			state = State::MovieName;
		} else {
			addMovie(std::move(line));
			state = State::SerieDelimiter;
		}
	};

	int lineNumber = 0;
	std::string line;

	while (std::getline(file, line)) {
		lineNumber++;

		switch (state) {
		case State::Category: {
			assert(category.empty());
			const bool ok = tryCategory(line);
			assert(ok);
			state = State::MovieName;
		}
			continue;

		case State::MovieName:
			assert(!line.empty());
			addMovie(std::move(line));
			state = State::SerieDelimiter;
			continue;

		case State::SerieDelimiter:
			if (!line.empty()) continue;
			state = State::SerieName;
			continue;

		case State::SerieName:
			if (line.empty()) continue;
			assert(serie.shortName.empty());
			splitSerieName(line);
// printf("se: %s -> %s\n", serie.shortName.c_str(), serie.fullName.c_str()); fflush(stdout);
			state = State::SerieSteps;
			continue;

		case State::SerieSteps: {
			if (line.empty()) {
				if (serie.steps.empty()) {
					continue;
				}
				completeSerie();
				state = State::SerieName;
				continue;
			}

			if (line == "Note that gorok has included a much shorter solution in the comments!") {
				std::getline(file, line);
				lineNumber++;
				continue;
			}

			std::vector<Dir> steps = [] (std::string line) -> std::vector<Dir> {
				std::for_each(line.begin(), line.end(), [] (char & c) { c = std::tolower(c); });
				const bool isDir = [&line] () -> bool {
					for (const Dir dir : kAllDirs) {
						const std::string_view name = nameForDir(dir);
						if (line.starts_with(name) && !std::isalpha(name[name.size()])) return true;
					}
					return false;
				}();
				if (!isDir) return {};

				std::vector<Dir> steps;
				std::string_view s = line;
				while (!s.empty()) {
					const auto eit = std::find_if(s.begin(), s.end(),
							[] (const char & c) { return !std::isalnum(c); });
					const std::string_view::size_type pos = std::distance(s.begin(), eit);
					const std::string_view token = s.substr(0, pos);
					const Dir dir = dirForName(token);
					assert(dir != kNullDir);
					steps.push_back(dir);

					const auto sit = std::find_if(eit, s.end(),
							[] (const char & c) { return std::isalnum(c); });
					s = s.substr(std::distance(s.begin(), sit));
				}

				return steps;
			}(line);

			if (!steps.empty()) {
				std::move(steps.begin(), steps.end(), std::back_inserter(serie.steps));
				continue;
			}

			state = State::Next;

			if (!serie.shortName.empty()) {
				if (serie.steps.empty()) {
					serie = {};
					next(std::move(title));
					continue;
				}
				completeSerie();
			}

			next(std::move(line));
		}
			continue;

		case State::Next:
			next(std::move(line));
			continue;
		}
	}

	completeMovie();

	return Moobaa {
		.movieForName = std::move(movieForName),
	};
}
