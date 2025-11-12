
#pragma once

#include <filesystem>

#include "Common.hpp"




struct Map {
	static Map load(const std::filesystem::path & path);
	static void draw(const Map & map);

	int width, height;
	std::vector<Wall> hwalls;
	std::vector<Wall> vwalls;
	std::vector<Trap> traps;
	std::vector<Phone> phones;
	std::vector<Gum> gums;
	std::vector<Teleport> teleports;
	Portal portal;
	State state;

	bool contains(const Pos & pos) const noexcept
	{
		return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height;
	}

	auto findGum(const Pos & pos) const noexcept
	{
		return std::find_if(gums.begin(), gums.end(), [&pos] (const Gum & gum) {
			return gum.pos == pos;
		});
	}

	bool hasGum(const Pos & pos) const noexcept
	{
		return findGum(pos) != gums.end();
	}

	auto findPhone(const Pos & pos) const noexcept
	{
		return std::find_if(phones.begin(), phones.end(), [&pos] (const Phone & phone) {
			return phone.pos == pos;
		});
	}

	const Phone & getPhone(const Pos & pos) const
	{
		const auto it = std::find_if(phones.begin(), phones.end(), [&pos] (const Phone & phone) {
			return phone.pos == pos;
		});
		assert(it != phones.end());
		return *it;
	}
};
