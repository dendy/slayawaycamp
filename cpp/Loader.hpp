
#pragma once

#include <filesystem>

#include <QString>

#include "Map.hpp"




class Loader {
public:
	Loader();

	Map load(const std::filesystem::path & path) const noexcept;
	std::vector<QString> convert(const Map & map) const noexcept;
	void draw(const Map & map) const noexcept;
	void save(const std::filesystem::path & path, const Map & map) const noexcept;

private:
	static Map _create(Map::Info && info, std::vector<QString> && lines) noexcept;
};
