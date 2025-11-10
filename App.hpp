
#pragma once

#include <algorithm>
#include <array>
#include <filesystem>
#include <queue>
#include <vector>
#include <cassert>
#include <string_view>




enum class Dir {
	Left, Right, Up, Down
};




enum class Orientation {
	Horz, Vert, Down,
};




enum class Color {
	Blue, Red, Yellow
};




struct Pos {
	int x, y;

	static Pos null() noexcept { return { -1, -1 }; }

	bool operator==(const Pos & other) const noexcept
	{
		return x == other.x && y == other.y;
	}

	bool operator!=(const Pos & other) const noexcept
	{
		return !operator==(other);
	}

	Pos operator+(const Pos & p) const noexcept
	{
		return Pos{x + p.x, y + p.y};
	}
};




template<>
struct std::hash<Pos> {
	std::size_t operator()(const Pos & pos) const noexcept
	{
		std::size_t hash = 0;
		hash ^= std::hash<int>{}(pos.x);
		hash ^= std::hash<int>{}(pos.y);
		return hash;
	}
};




inline Pos shiftForDir(const Dir dir) noexcept
{
	switch (dir) {
	case Dir::Left:  return { -1,  0 };
	case Dir::Right: return {  1,  0 };
	case Dir::Up:    return {  0, -1 };
	case Dir::Down:  return {  0,  1} ;
	}
	assert(false);
}


inline std::string_view nameForDir(const Dir & dir) noexcept
{
	switch (dir) {
	case Dir::Left:  return "left";
	case Dir::Right: return "right";
	case Dir::Up:    return "up";
	case Dir::Down:  return "down";
	}
	assert(false);
}


inline char shortNameForDir(const Dir & dir) noexcept
{
	switch (dir) {
	case Dir::Left:  return 'l';
	case Dir::Right: return 'r';
	case Dir::Up:    return 'u';
	case Dir::Down:  return 'd';
	}
	assert(false);
}


inline std::string_view nameForOrientation(const Orientation & orientation) noexcept
{
	switch (orientation) {
	case Orientation::Horz: return "horz";
	case Orientation::Vert: return "vert";
	case Orientation::Down: return "down";
	}
	assert(false);
}


inline Dir oppositeDir(const Dir & dir) noexcept
{
	switch (dir) {
	case Dir::Left:  return Dir::Right;
	case Dir::Right: return Dir::Left;
	case Dir::Up:    return Dir::Down;
	case Dir::Down:  return Dir::Up;
	}
	assert(false);
}


inline bool dirMatchesOrientation(const Dir dir, const Orientation orientation)
{
	switch (dir) {
	case Dir::Left:
	case Dir::Right:
		return orientation == Orientation::Horz;
	case Dir::Up:
	case Dir::Down:
		return orientation == Orientation::Vert;
	}
	assert(false);
}




struct Killer {
	Pos pos;

	bool operator==(const Killer & other) const noexcept
	{
		return pos == other.pos;
	}

	bool operator!=(const Killer & other) const noexcept
	{
		return pos != other.pos;
	}
};




struct Dude {
	enum class Type {
		Victim,
		Cat,
		Cop,
		Swat,
		Drop,
	};

	Type type;
	Pos pos;
	Dir dir = Dir::Left;
	Orientation orientation = Orientation::Horz;
	Color color = Color::Blue;

	bool operator<(const Dude & other) const noexcept
	{
		if (pos.y < other.pos.y) return true;
		if (pos.y > other.pos.y) return false;
		return pos.x < other.pos.x;
	}

	bool operator==(const Dude & other) const noexcept
	{
		if (type != other.type) return false;
		if (pos != other.pos) return false;
		if (type == Type::Cop || type == Type::Swat) {
			if (dir != other.dir) return false;
		}
		return true;
	}
};




inline std::string_view nameForDudeType(const Dude::Type & type) noexcept
{
	switch (type) {
	case Dude::Type::Victim: return "victim";
	case Dude::Type::Cat: return "cat";
	case Dude::Type::Cop: return "cop";
	case Dude::Type::Swat: return "swat";
	case Dude::Type::Drop: return "drop";
	}
	assert(false);
}




struct Mine {
	Pos pos;

	bool operator<(const Mine & other) const noexcept
	{
		if (pos.y < other.pos.y) return true;
		if (pos.y > other.pos.y) return false;
		return pos.x < other.pos.x;
	}

	bool operator==(const Mine & other) const noexcept
	{
		return pos == other.pos;
	}
};




struct Trap {
	Pos pos;

	bool operator==(const Trap & other) const noexcept
	{
		return pos == other.pos;
	}
};




struct Phone {
	Pos pos;
	Color color;
};




struct Portal {
	Pos pos;
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
		return pos == other.pos;
	}
};




struct Gum {
	Pos pos;
};




struct Teleport {
	Pos pos;
	Color color;
};




struct State {
	Killer killer;
	std::vector<Dude> dudes;
	std::vector<Mine> mines;
	bool light = true;

	const auto findDude(const Pos & pos) const noexcept
	{
		return std::find_if(dudes.begin(), dudes.end(), [&pos] (const Dude & dude) {
			return dude.pos == pos;
		});
	}

	const Dude & getDude(const Pos & pos) const noexcept
	{
		const auto it = std::find_if(dudes.begin(), dudes.end(), [&pos] (const Dude & dude) {
			return dude.pos == pos;
		});
		assert(it != dudes.end());
		return *it;
	}

	bool hasVictims() const noexcept
	{
		for (const Dude & dude : dudes) {
			if (dude.type == Dude::Type::Victim) return true;
		}
		return false;
	}

	void moveDude(const Dude & dude, Dude && target) noexcept
	{
		{
			const auto it = std::find(dudes.begin(), dudes.end(), dude);
			assert(it != dudes.end());
			dudes.erase(it);
		}

		const auto it = std::find_if(dudes.begin(), dudes.end(), [&target] (const Dude & d) {
			return target < d;
		});
		dudes.insert(it, std::move(target));
	}

	bool operator==(const State & other) const noexcept
	{
		if (killer != other.killer) return false;
		if (dudes != other.dudes) return false;
		if (mines != other.mines) return false;
		if (light != other.light) return false;
		return true;
	}
};


template<>
struct std::hash<State> {
	std::size_t operator()(const State & state) const noexcept
	{
		std::size_t hash = 0;
		hash ^= std::hash<Pos>{}(state.killer.pos);
		for (const Dude & dude : state.dudes) {
			hash ^= std::hash<Dude::Type>{}(dude.type);
			hash ^= std::hash<Pos>{}(dude.pos);
			if (dude.type == Dude::Type::Cop || dude.type == Dude::Type::Swat) {
				hash ^= std::hash<Dir>{}(dude.dir);
			}
			if (dude.type == Dude::Type::Drop) {
				hash ^= std::hash<Orientation>{}(dude.orientation);
			}
		}
		for (const Mine & mine : state.mines) {
			hash ^= std::hash<Pos>{}(mine.pos);
		}
		hash ^= std::hash<bool>{}(state.light);
		return hash;
	}
};




struct Map {
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

	auto findPhone(const Pos & pos) const
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




struct Move {
	Dir dir;
	int previousId;
	State state;
};




inline static constexpr std::initializer_list<Dir> kAllDirs = {Dir::Left, Dir::Right, Dir::Up, Dir::Down};
inline static constexpr std::initializer_list<Color> kAllColors = {Color::Blue, Color::Red, Color::Yellow};




class App {
public:
	inline static constexpr int kMaxTeleportCount = kAllColors.size() * 2;

	enum class Bump {
		Wall,
		Dude,
		Drop,
		Phone,
		Death,
		Portal,
		Gum,
	};

	struct Res {
		Bump bump;
		Pos pos;
		Pos target;
		std::array<Teleport, kMaxTeleportCount> teleports;
	};

	struct Extra {
		struct Dropped {
			Dude drop;
			Dir dir;
		};
		struct Scared {
			Dude dude;
			Dir dir;
		};
		struct Called {
			Dude dude;
			Dir dir;
		};
		enum class Fail {
			None, Catality, Escaped, Cop, Swat
		};
		enum class Win {
			None, Switch
		};
		std::queue<Dude> killed;
		std::queue<Dropped> dropped;
		std::queue<Scared> scared;
		std::queue<Called> called;
		Fail fail = Fail::None;
		Win win = Win::None;
	};

	App(const std::filesystem::path & path);

	static Map load(const std::filesystem::path & path);

	const Teleport & getOtherTeleport(const Teleport & teleport) const noexcept;
	const Wall * findWall(const Pos & pos, Dir dir) const noexcept;
	const Wall & getWall(const Pos & pos, Dir dir) const noexcept;
	bool hasAnyWall(const Pos & pos, Dir dir) const noexcept;
	bool hasTallWall(const Pos & pos, Dir dir) const noexcept;
	void trySwitchLight(State & state, const Wall & wall, Dir dir, Extra & extra) noexcept;
	Res go(State & state, const Pos & fromPos, Dir dir, bool portal) const noexcept;
	bool aimedByCop(const State & state, const Dude & cop) const noexcept;
	bool aimedByAnyCop(const State & state) const noexcept;
	bool aimedBySwat(const State & state, const Dude & dude) const noexcept;
	bool aimedByAnySwat(const State & state) const noexcept;
	void scare(const State & state, const Pos & pos, Extra & extra) const;
	void call(const State & state, const Dude & who, const Phone & phone, Extra & extra) const;
	void goDude(State & state, Dude dude, Dir dir, bool called, Extra & extra);
	void kill(State & state, Dude dude, Extra & extra, std::queue<Extra::Scared> & scared);
	void processExtra(State & state, Extra & extra);

	void exec() noexcept;

	const Map map;
};
