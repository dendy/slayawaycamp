
#include "Map.hpp"




const Wall * Map::findWall(const Pos & pos, const Dir dir) const noexcept
{
	const Pos shift = shiftForDir(dir);
	const std::vector<Wall> & walls = shift.x != 0 ? vwalls : hwalls;
	const int shiftDist = shift.x != 0 ? shift.x : shift.y;
	const int wallShift = shiftDist == -1 ? 0 : 1;

	const auto wallPos = [&shift, &pos, wallShift] () -> Pos {
		if (shift.x != 0) {
			return Pos {
				.x = pos.x + wallShift,
				.y = pos.y,
			};
		} else {
			return Pos {
				.x = pos.x,
				.y = pos.y + wallShift,
			};
		}
	}();

	const auto it = std::find_if(walls.begin(), walls.end(), [&wallPos] (const Wall & wall) {
		return wall.pos == wallPos;
	});
	if (it == walls.end()) {
		return nullptr;
	} else {
		return &*it;
	}
}


const Wall & Map::getWall(const Pos & pos, const Dir dir) const noexcept
{
	const Wall * const pwall = findWall(pos, dir);
	assert(pwall);
	return *pwall;
}


bool Map::hasAnyWall(const Pos & pos, const Dir dir) const noexcept
{
	const Wall * const pwall = findWall(pos, dir);
	return pwall != nullptr;
}


bool Map::hasTallWall(const Pos & pos, const Dir dir) const noexcept
{
	const Wall * const pwall = findWall(pos, dir);
	if (!pwall) {
		return false;
	} else {
		switch (pwall->type) {
		case Wall::Type::Normal:
		case Wall::Type::Switch:
			return true;
		case Wall::Type::Escape:
		case Wall::Type::Short:
		case Wall::Type::Zap:
			return false;
		}
		assert(false);
	}
}


const Teleport & Map::getOtherTeleport(const Teleport & teleport) const noexcept
{
	const auto it = std::find_if(teleports.begin(), teleports.end(),
			[&teleport] (const Teleport & t) {
				if (t.pos == teleport.pos) return false;
				return t.color == teleport.color;
			});
	assert(it != teleports.end());
	return *it;
}
