
#include "Player.hpp"




Player::Result Player::play(const Map & map, State & state, const Dir dir)
{
	return Player(map, state)._step(dir);
}


Player::Player(const Map & map, State & state) :
	map_(map),
	state_(state)
{
}


bool Player::_aimedByCop(const Dude & cop) const noexcept
{
	assert(cop.type == Dude::Type::Cop);
	if (!state_.light) {
		// cops cannot aim when lights are off
		return false;
	}
	if (map_.hasAnyWall(cop.pos, cop.dir)) {
		// cannot aim through any wall
		return false;
	}
	return cop.pos + shiftForDir(cop.dir) == state_.killer.pos;
}


bool Player::_aimedByAnyCop() const noexcept
{
	for (const Dude & dude : state_.dudes) {
		if (dude.type == Dude::Type::Cop) {
			if (_aimedByCop(dude)) {
				return true;
			}
		}
	}
	return false;
}


bool Player::_aimedBySwat(const Dude & swat) const noexcept
{
	Pos pos = swat.pos;

	while (true) {
		if (map_.hasTallWall(pos, swat.dir)) {
			break;
		}

		const Pos nextPos = pos + shiftForDir(swat.dir);

		if (nextPos == state_.killer.pos) {
			return true;
		}

		{
			const auto it = map_.findPhone(nextPos);
			if (it != map_.phones.end()) {
				break;
			}
		}

		{
			const auto it = state_.findDude(nextPos);
			if (it != state_.dudes.end()) {
				break;
			}
		}

		pos = nextPos;
	}

	return false;
}


bool Player::_aimedByAnySwat() const noexcept
{
	for (const Dude & dude : state_.dudes) {
		if (dude.type == Dude::Type::Swat) {
			if (_aimedBySwat(dude)) return true;
		}
	}
	return false;
}


void Player::_trySwitchLight(const Wall & wall, const Dir dir, Extra & extra) noexcept
{
	if (wall.type == Wall::Type::Switch) {
		if (dir == Dir::Up || dir == Dir::Left) {
			state_.light = !state_.light;
			if (wall.win) {
				extra.win = Extra::Win::Switch;
			}
		}
	}
}


Player::Res Player::_go(const Pos & fromPos, const Dir dir, const bool portal) noexcept
{
	std::array<Teleport, kMaxTeleportCount> visitedTeleports;
	std::for_each(visitedTeleports.begin(), visitedTeleports.end(),
			[] (Teleport & t) { t.pos = Pos::null(); });

	const auto addTeleport = [&visitedTeleports] (const Teleport & teleport) {
		const auto it = std::find_if(visitedTeleports.begin(), visitedTeleports.end(),
				[] (const Teleport & t) { return t.pos == Pos::null(); });
		assert(it != visitedTeleports.end());
		*it = teleport;
	};

	const Pos shift = shiftForDir(dir);

	Pos pos = fromPos;

	while (true) {
		const Pos nextPos = pos + shift;

		const auto makeRes = [&pos, &nextPos, &visitedTeleports] (
				const Bump & bump) -> Res {
			return Res {
				.bump = bump,
				.pos = pos,
				.target = nextPos,
				.teleports = std::move(visitedTeleports),
			};
		};

		if (map_.hasAnyWall(pos, dir)) {
			const Wall & wall = map_.getWall(pos, dir);
			if (state_.light && wall.type == Wall::Type::Zap) {
				// bumped into electric wire wall
				return makeRes(Bump::Death);
			}

			// bumped into a wall
			return makeRes(Bump::Wall);
		}

		{
			const auto it = state_.findDude(nextPos);
			if (it != state_.dudes.end()) {
				// bumped into dude
				const Dude & dude = *it;

				if (dude.type == Dude::Type::Drop) {
					return makeRes(Bump::Drop);
				} else {
					return makeRes(Bump::Dude);
				}
			}
		}

		{
			const auto it = map_.findPhone(nextPos);
			if (it != map_.phones.end()) {
				return makeRes(Bump::Phone);
			}
		}

		pos = nextPos;

		{
			const Trap trap = Trap{nextPos};
			const auto it = std::find(map_.traps.begin(), map_.traps.end(), trap);
			if (it != map_.traps.end()) {
				// got into a trap
				return makeRes(Bump::Death);
			}
		}

		{
			const Mine mine = Mine{nextPos};
			const auto it = std::find(state_.mines.begin(), state_.mines.end(), mine);
			if (it != state_.mines.end()) {
				// stepped on a mine
				state_.mines.erase(it);
				return makeRes(Bump::Death);
			}
		}

		{
			const auto it = std::find_if(map_.teleports.begin(), map_.teleports.end(),
					[&pos] (const Teleport & teleport) {
						return teleport.pos == pos;
					});
			if (it != map_.teleports.end()) {
				const Teleport & teleport = *it;
				const Teleport & otherTeleport = map_.getOtherTeleport(teleport);
				addTeleport(teleport);
				addTeleport(otherTeleport);
				pos = otherTeleport.pos;
				continue;
			}
		}

		if (portal && pos == map_.portal.pos) {
			return makeRes(Bump::Portal);
		}

		{
			const auto it = map_.findGum(nextPos);
			if (it != map_.gums.end()) {
				return makeRes(Bump::Gum);
			}
		}
	}

	assert(false);
}


void Player::_scare(const Pos & pos, Extra & extra) const noexcept
{
	for (const Dir dir : kAllDirs) {
		if (map_.hasTallWall(pos, dir)) {
			continue;
		}

		const auto it = state_.findDude(pos + shiftForDir(dir));
		if (it == state_.dudes.end()) {
			// no dude here
			continue;
		}

		const Dude & dude = *it;
		if (dude.type != Dude::Type::Victim && dude.type != Dude::Type::Cat) {
			// can scare only victims and cats
			continue;
		}

		if (dude.type == Dude::Type::Victim) {
			if (!state_.light) {
				// cannot scare victim when lights are off, but cats still see in the dark
				continue;
			}
		}

		extra.scared.push(Extra::Scared {
			.dude = dude,
			.dir = dir,
		});
	}
}


void Player::_call(const Dude & who, const Phone & phone, Extra & extra) const noexcept
{
	for (const Phone & otherPhone : map_.phones) {
		if (otherPhone.pos == phone.pos) continue;
		if (otherPhone.color != phone.color) continue;

		for (const Dir dir : kAllDirs) {
			Pos pos = otherPhone.pos;

			while (true) {
				if (map_.hasTallWall(pos, dir)) {
					break;
				}

				const Pos nextPos = pos + shiftForDir(dir);

				const auto it = state_.findDude(nextPos);
				if (it != state_.dudes.end()) {
					const Dude & dude = *it;
					if (dude.pos != who.pos) {
						switch (dude.type) {
						case Dude::Type::Victim:
						case Dude::Type::Cop:
						case Dude::Type::Swat:
							extra.called.push(Extra::Called {
								.dude = dude,
								.dir = oppositeDir(dir),
							});
							break;
						case Dude::Type::Cat:
							extra.scared.push(Extra::Scared {
								.dude = dude,
								.dir = dir,
							});
							break;
						case Dude::Type::Drop:
							break;
						default:
							assert(false);
						}
					}
					break;
				}

				pos = nextPos;
			}
		}
	}
}


void Player::_goDude(const Dude dude, const Dir dir, const bool called, Extra & extra) noexcept
{
	const Res res = _go(dude.pos, dir, false);

	switch (res.bump) {
	case Bump::Wall:
	case Bump::Dude:
	case Bump::Death:
	case Bump::Drop:
	case Bump::Phone:
	case Bump::Gum: {
		const Dude target = Dude {
			.type = dude.type,
			.pos = res.pos,
			.dir = dir,
		};
		state_.moveDude(dude, Dude(target));
		if (res.bump == Bump::Death) {
			extra.killed.push(target);
		}
		if (res.bump == Bump::Drop) {
			const Dude & drop = state_.getDude(res.target);
			assert(drop.type == Dude::Type::Drop);
			if (dirMatchesOrientation(dir, drop.orientation)) {
				extra.dropped.push(Extra::Dropped {
					.drop = drop,
					.dir = dir,
				});
			}
		}
		if (res.bump == Bump::Phone) {
			// bumped into a phone, lets call it
			if (dude.type != Dude::Type::Cat) { // cats cannit call
				if (!called) { // cannot call to myself
					_call(target, map_.getPhone(res.target), extra);
				}
			}
		}
		if (res.bump == Bump::Wall) {
			const Wall & wall = map_.getWall(target.pos, dir);
			_trySwitchLight(wall, dir, extra);
			if (wall.type == Wall::Type::Escape) {
				if (dude.type == Dude::Type::Victim || dude.type == Dude::Type::Cat) {
					extra.fail = Extra::Fail::Escaped;
				}
			}
		}
	}
		break;

	case Bump::Portal:
		break;
	}
}


void Player::_kill(const Dude dude, Extra & extra, std::queue<Extra::Scared> & scared) noexcept
{
	{
		const auto it = state_.findDude(dude.pos);
		assert(it != state_.dudes.end());
		const Dude & dude = *it;
		if (dude.type == Dude::Type::Cat) {
			extra.fail = Extra::Fail::Catality;
		}
		state_.dudes.erase(it);
	}

	{
		std::queue<Extra::Scared> nextScared;
		while (!scared.empty()) {
			const Extra::Scared s = scared.front();
			scared.pop();
			if (s.dude.pos != dude.pos) {
				nextScared.push(s);
			}
		}
		std::swap(scared, nextScared);
	}

	_scare(dude.pos, extra);
}


void Player::_processExtra(Extra & extra) noexcept
{
	// check if we stopped in front of a cop
	if (_aimedByAnyCop()) {
		extra.fail = Extra::Fail::Cop;
		return;
	}

	// check if we are on a line sight of a swat
	if (_aimedByAnySwat()) {
		extra.fail = Extra::Fail::Swat;
		return;
	}

	if (extra.killed.empty() && extra.dropped.empty() && extra.scared.empty() &&
			extra.called.empty()) {
		return;
	}

	Extra nextExtra;

	while (!extra.killed.empty()) {
		const Dude dude = extra.killed.front();
		extra.killed.pop();
		_kill(dude, nextExtra, extra.scared);
	}

	while (!extra.dropped.empty()) {
		const Extra::Dropped dropped = extra.dropped.front();
		extra.dropped.pop();

		if (map_.hasAnyWall(dropped.drop.pos, dropped.dir)) {
			// cannot drop onto the wall
			continue;
		}

		const Pos dropPos = dropped.drop.pos + shiftForDir(dropped.dir);
		const auto it = state_.findDude(dropPos);
		if (it != state_.dudes.end()) {
			const Dude & dude = *it;
			if (dude.type == Dude::Type::Drop) {
				// cannot drop onto another drop
				continue;
			}
			_kill(dude, nextExtra, extra.scared);
		}

		if (dropPos == state_.killer.pos) {
			extra.fail = Extra::Fail::Drop;
			continue;
		}

		state_.moveDude(dropped.drop, Dude {
			.type = Dude::Type::Drop,
			.pos = dropPos,
			.orientation = Orientation::Down,
		});
	}

	while (!extra.scared.empty()) {
		const Extra::Scared scared = extra.scared.front();
		extra.scared.pop();
		_goDude(scared.dude, scared.dir, false, nextExtra);
	}

	while (!extra.called.empty()) {
		const Extra::Called called = extra.called.front();
		extra.called.pop();
		_goDude(called.dude, called.dir, true, nextExtra);
	}

	_processExtra(nextExtra);

	if (nextExtra.fail != Extra::Fail::None) {
		extra.fail = nextExtra.fail;
	}
	if (nextExtra.win != Extra::Win::None) {
		extra.win = nextExtra.win;
	}
}


Player::Result Player::_step(const Dir dir)
{
	bool fail = false;
	bool win = false;

	const Res res = _go(state_.killer.pos, dir, !state_.hasVictims());

	Extra extra;

	for (const Teleport & teleport : res.teleports) {
		if (teleport.pos == Pos::null()) break;
		if (teleport.pos != res.pos) {
			_scare(teleport.pos, extra);
		}
	}

	switch (res.bump) {
	case Bump::Gum : {
		state_.killer.pos = res.pos;
		_scare(state_.killer.pos, extra);
	}
		break;

	case Bump::Wall: {
		const Wall & wall = map_.getWall(res.pos, dir);
		_trySwitchLight(wall, dir, extra);

		state_.killer.pos = res.pos;
		_scare(state_.killer.pos, extra);
	}
		break;

	case Bump::Dude: {
		state_.killer.pos = res.pos;

		const Dude dude = state_.getDude(res.target);

		if (dude.type == Dude::Type::Cop) {
			if (_aimedByCop(dude)) {
				fail = true;
				break;
			}
		}

		if (dude.type == Dude::Type::Swat) {
			if (state_.light) {
				// swat kills you instantly when lights are on
				fail = true;
				break;
			}
		}

		_scare(state_.killer.pos, extra);
		extra.killed.push(dude);
	}
		break;

	case Bump::Drop: {
		state_.killer.pos = res.pos;

		const Dude drop = state_.getDude(res.target);

		_scare(state_.killer.pos, extra);
		if (dirMatchesOrientation(dir, drop.orientation)) {
			extra.dropped.push(Extra::Dropped {
				.drop = drop,
				.dir = dir,
			});
		}
	}
		break;

	case Bump::Phone: {
		state_.killer.pos = res.pos;

		const Phone & phone = map_.getPhone(res.target);

		_scare(state_.killer.pos, extra);
		_call(Dude{.type = Dude::Type::Victim, .pos = state_.killer.pos}, phone, extra);
	}
		break;

	case Bump::Death:
		fail = true;
		break;

	case Bump::Portal:
		state_.killer.pos = res.pos;
		win = true;
		break;
	}

	switch (res.bump) {
	case Bump::Wall:
	case Bump::Dude:
	case Bump::Drop:
	case Bump::Phone:
	case Bump::Gum: {
		// normal bump
		_processExtra(extra);
	}
		break;

	case Bump::Death:
	case Bump::Portal:
		break;
	}

	if (extra.fail != Extra::Fail::None || fail) {
		return Result::Fail;
	}

	if (extra.win != Extra::Win::None || win) {
		return Result::Win;
	}

	if (!win) {
		if (!state_.hasVictims()) {
			if (map_.portal.pos == Pos::null()) {
				return Result::Win;
			}
		}
	}

	return Result::None;
}
