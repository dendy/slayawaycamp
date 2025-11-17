
#pragma once

#include <filesystem>
#include <unordered_map>

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
	struct CornerForTag {
		std::vector<char> tags;
		std::unordered_map<std::string_view, QStringView> map;
	};

	static CornerForTag _createCornerForTag() noexcept;
	static Map _createMap(Map::Info && info, std::vector<QString> && lines) noexcept;

	const CornerForTag cornerForTag_ = _createCornerForTag();
};
