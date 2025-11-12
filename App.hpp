
#pragma once

#include <filesystem>
#include <queue>
#include <cassert>

#include "Common.hpp"
#include "Map.hpp"




class App {
public:
	inline static constexpr int kMaxTeleportCount = kAllColors.size() * 4;

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
