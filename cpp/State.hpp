
#pragma once

#include "Common.hpp"




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


struct Hash {
	std::size_t sum = 0;
	int index = 0;

	template <typename T>
	void operator()(const T & v) noexcept
	{
		sum ^= (std::hash<T>{}(v) << index);
		index++;
		if (index == sizeof(sum)) index = 0;
	};
};


template<>
struct std::hash<State> {
	std::size_t operator()(const State & state) const noexcept
	{
		Hash hash;
		hash(state.killer.pos);
		for (const Dude & dude : state.dudes) {
			hash(dude.type);
			hash(dude.pos);
			if (dude.type == Dude::Type::Cop || dude.type == Dude::Type::Swat) {
				hash(dude.dir);
			}
			if (dude.type == Dude::Type::Drop) {
				hash(dude.orientation);
			}
		}
		for (const Mine & mine : state.mines) {
			hash(mine.pos);
		}
		hash(state.light);
		return hash.sum;
	}
};
