
#include <filesystem>
#include <stdexcept>

#include "App.hpp"




static std::filesystem::path getLastFilePath()
{
	std::filesystem::file_time_type time;
	std::filesystem::path last;

	for (const std::filesystem::directory_entry & entry :
			std::filesystem::recursive_directory_iterator(SLAYAWAYCAMP_LEVELS_DIR)) {
		if (entry.is_regular_file() && entry.path().extension() == ".camp") {
			const std::filesystem::file_time_type entryTime = std::filesystem::last_write_time(entry);
			if (last.empty() || entryTime > time) {
				last = entry;
				time = entryTime;
			}
		}
	}

	if (last.empty()) {
		throw std::runtime_error("No last level file found");
	}

	return last;
}


static std::filesystem::path getLevelFilePath(const std::string_view & name)
{
	if (name == "last") {
		return getLastFilePath();
	} else {
		const std::filesystem::path path = name;
		if (path.is_absolute()) {
			return path;
		} else {
			return std::filesystem::path(SLAYAWAYCAMP_LEVELS_DIR) / path;
		}
	}
}


int main(int argc, char ** argv)
{
	if (argc != 2) {
		throw std::runtime_error("Usage: slayawaycamp <level>");
	}

	const std::filesystem::path levelFilePath = getLevelFilePath(argv[1]);

	App app(levelFilePath);

	return 0;
}
