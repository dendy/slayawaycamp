
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
	struct BoxProfile {
		bool hasSingle;
		QChar h, v;
	};

	struct CornerForTag {
		std::unordered_map<uint64_t, QChar> corners;
	};

	using Tag = QChar[4];

	static const BoxProfile kLightBoxProfile;
	static const BoxProfile kHeavyBoxProfile;
	static const BoxProfile kDoubleBoxProfile;

	static QChar _profileCharForDir(const Loader::BoxProfile & profile, Dir dir) noexcept;
	static uint64_t _keyForTag(const Tag & tag) noexcept;
	static CornerForTag _createCornerForTag() noexcept;
	static Map _createMap(Map::Info && info, std::vector<QString> && lines) noexcept;

	const CornerForTag cornerForTag_ = _createCornerForTag();
	const BoxProfile & normalWallBoxProfile_;
	const BoxProfile & shortWallBoxProfile_;
};
