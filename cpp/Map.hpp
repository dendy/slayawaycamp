
#pragma once

#include <filesystem>

#include "State.hpp"




class QString;




struct Phone {
	Pos pos;
	Color color;

	bool operator==(const Phone & other) const noexcept
	{
		return pos == other.pos && color == other.color;
	}
};




struct Portal {
	Pos pos;

	bool operator==(const Portal & other) const noexcept
	{
		return pos == other.pos;
	}
};




struct Wall {
	enum class Type {
		Normal,
		Escape,
		Switch,
		Short,
		Zap,
	};

	Type type;
	Pos pos;
	bool win = false;

	bool operator==(const Wall & other) const noexcept
	{
		return pos == other.pos && type == other.type && win == other.win;
	}
};




struct Gum {
	Pos pos;

	bool operator==(const Gum & other) const noexcept
	{
		return pos == other.pos;
	}
};




struct Teleport {
	Pos pos;
	Color color;

	bool operator==(const Teleport & other) const noexcept
	{
		return pos == other.pos && color == other.color;
	}
};




struct Trap {
	Pos pos;

	bool operator==(const Trap & other) const noexcept
	{
		return pos == other.pos;
	}
};




struct Map {
	struct Info {
		std::string shortName;
		std::string fullName;
		std::string moobaaName;
		int turns = -1;
	};

	Info info;
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

	auto findTeleport(const Pos & pos) const noexcept
	{
		return std::find_if(teleports.begin(), teleports.end(), [&pos] (const Teleport & teleport) {
			return teleport.pos == pos;
		});
	}

	const Wall * findWall(const Pos & pos, Dir dir) const noexcept;
	const Wall & getWall(const Pos & pos, Dir dir) const noexcept;
	bool hasAnyWall(const Pos & pos, Dir dir) const noexcept;
	bool hasTallWall(const Pos & pos, Dir dir) const noexcept;
	const Teleport & getOtherTeleport(const Teleport & teleport) const noexcept;

	bool operator==(const Map & other) const noexcept;
};




inline bool Map::operator==(const Map & other) const noexcept
{
	// if (info != other.info) return false;
	if (width != other.width) return false;
	if (height != other.height) return false;
	if (hwalls != other.hwalls) return false;
	if (vwalls != other.vwalls) return false;
	if (traps != other.traps) return false;
	if (phones != other.phones) return false;
	if (gums != other.gums) return false;
	if (teleports != other.teleports) return false;
	if (portal != other.portal) return false;
	if (state != other.state) return false;
	return true;
}
