
#pragma once

#include <queue>

#include "Map.hpp"




class Player {
public:
	enum class Result {
		None,
		Fail,
		Win,
	};

	static Result play(const Map & map, State & state, Dir dir);

private:
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

	Player(const Map & map, State & state);

	Result _step(Dir dir);

	bool _aimedByCop(const Dude & cop) const noexcept;
	bool _aimedByAnyCop() const noexcept;
	bool _aimedBySwat(const Dude & swat) const noexcept;
	bool _aimedByAnySwat() const noexcept;

	void _trySwitchLight(const Wall & wall, Dir dir, Extra & extra) noexcept;
	Res _go(const Pos & fromPos, Dir dir, bool portal) noexcept;
	void _scare(const Pos & pos, Extra & extra) const noexcept;
	void _call(const Dude & who, const Phone & phone, Extra & extra) const noexcept;
	void _goDude(Dude dude, Dir dir, bool called, Extra & extra) noexcept;
	void _kill(Dude dude, Extra & extra, std::queue<Extra::Scared> & scared) noexcept;
	void _processExtra(Extra & extra) noexcept;

	const Map & map_;

	State & state_;
};
