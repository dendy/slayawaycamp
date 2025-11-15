
#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <queue>
#include <vector>
#include <string_view>




enum class Dir {
	Left, Right, Up, Down
};

inline static constexpr Dir kNullDir = Dir(-1);

using Steps = std::vector<Dir>;




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


inline Dir dirForName(const std::string_view & name) noexcept
{
	if (name == "left") return Dir::Left;
	if (name == "right") return Dir::Right;
	if (name == "up") return Dir::Up;
	if (name == "down") return Dir::Down;
	return kNullDir;
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




enum class Category {
	Base, NC17, Xtra
};


inline std::string_view nameForCategory(const Category & category) noexcept
{
	switch (category) {
	case Category::Base: return "base";
	case Category::NC17: return "nc17";
	case Category::Xtra: return "xtra";
	}
	assert(false);
}




inline std::string stepsToString(const Steps & steps) noexcept
{
	std::string s;
	for (const Dir dir: steps) {
		s += shortNameForDir(dir);
	}
	return s;
}




inline std::string makeLower(const std::string_view & sv) noexcept
{
	std::string s(sv);
	std::for_each(s.begin(), s.end(), [] (char & c) { c = std::tolower(c); });
	return s;
}


template <typename T>
using RemoveFromQueueOp = std::function<bool(const T & item)>;

template <typename T>
inline void removeFromQueue(std::queue<T> & queue, const RemoveFromQueueOp<T> & op) noexcept
{
	std::queue<T> next;
	while (!queue.empty()) {
		T item = queue.front();
		queue.pop();
		if (!op(item)) {
			next.push(std::move(item));
		}
	}
	std::swap(queue, next);
}



inline static constexpr std::initializer_list<Dir> kAllDirs =
		{Dir::Left, Dir::Right, Dir::Up, Dir::Down};
inline static constexpr std::initializer_list<Color> kAllColors =
		{Color::Blue, Color::Red, Color::Yellow};
inline static constexpr std::initializer_list<Category> kAllCategories =
		{Category::Base, Category::NC17, Category::Xtra};

inline static constexpr int kMaxTeleportCount = kAllColors.size() * 4;

inline static constexpr std::string_view kSerieExtension = ".camp";
