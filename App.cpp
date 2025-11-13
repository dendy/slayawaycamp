
#include "App.hpp"

#include <cstring>




App::App(Args && args) :
	moobaa([&args] () -> Moobaa {
		if (args.moobaa) {
			return Moobaa::load();
		} else {
			return {};
		}
	}()),
	map([&args] () -> Map {
		if (!args.mapFilePath.empty()) {
			return Map::load(args.mapFilePath);
		} else {
			return {};
		}
	}())
{
	if (args.moobaa) {
		for (const auto it : moobaa.movieForName) {
			const Moobaa::Movie & movie = it.second;
			printf("movie: %s\n", movie.name.c_str());
		}
	}

	if (!map.shortName.empty()) {
		Map::draw(map);
		exec();
	}
}


const Teleport & App::getOtherTeleport(const Teleport & teleport) const noexcept
{
	const auto it = std::find_if(map.teleports.begin(), map.teleports.end(),
			[&teleport] (const Teleport & t) {
				if (t.pos == teleport.pos) return false;
				return t.color == teleport.color;
			});
	assert(it != map.teleports.end());
	return *it;
}


void App::trySwitchLight(State & state, const Wall & wall, const Dir dir, Extra & extra) noexcept
{
	if (wall.type == Wall::Type::Switch) {
		if (dir == Dir::Up || dir == Dir::Left) {
			state.light = !state.light;
			if (wall.win) {
				extra.win = Extra::Win::Switch;
			}
		}
	}
}


App::Res App::go(State & state, const Pos & fromPos, const Dir dir, const bool portal) const noexcept
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

		if (map.hasAnyWall(pos, dir)) {
			const Wall & wall = map.getWall(pos, dir);
			if (state.light && wall.type == Wall::Type::Zap) {
				// bumped into electric wire wall
				return makeRes(Bump::Death);
			}

			// bumped into a wall
			return makeRes(Bump::Wall);
		}

		{
			const auto it = state.findDude(nextPos);
			if (it != state.dudes.end()) {
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
			const auto it = map.findPhone(nextPos);
			if (it != map.phones.end()) {
				return makeRes(Bump::Phone);
			}
		}

		pos = nextPos;

		{
			const Trap trap = Trap{nextPos};
			const auto it = std::find(map.traps.begin(), map.traps.end(), trap);
			if (it != map.traps.end()) {
				// got into a trap
				return makeRes(Bump::Death);
			}
		}

		{
			const Mine mine = Mine{nextPos};
			const auto it = std::find(state.mines.begin(), state.mines.end(), mine);
			if (it != state.mines.end()) {
				// steppen on a mine
				state.mines.erase(it);
				return makeRes(Bump::Death);
			}
		}

		{
			const auto it = std::find_if(map.teleports.begin(), map.teleports.end(),
					[&pos] (const Teleport & teleport) {
						return teleport.pos == pos;
					});
			if (it != map.teleports.end()) {
				const Teleport & teleport = *it;
				const Teleport & otherTeleport = getOtherTeleport(teleport);
				addTeleport(teleport);
				addTeleport(otherTeleport);
				pos = otherTeleport.pos;
				continue;
			}
		}

		if (portal && pos == map.portal.pos) {
			return makeRes(Bump::Portal);
		}

		{
			const auto it = map.findGum(nextPos);
			if (it != map.gums.end()) {
				return makeRes(Bump::Gum);
			}
		}
	}

	assert(false);
}


bool App::aimedByCop(const State & state, const Dude & cop) const noexcept
{
	assert(cop.type == Dude::Type::Cop);
	if (!state.light) {
		// cops cannot aim when lights are off
		return false;
	}
	if (map.hasAnyWall(cop.pos, cop.dir)) {
		// cannot aim through any wall
		return false;
	}
	return cop.pos + shiftForDir(cop.dir) == state.killer.pos;
}


bool App::aimedByAnyCop(const State & state) const noexcept
{
	for (const Dude & dude : state.dudes) {
		if (dude.type == Dude::Type::Cop) {
			if (aimedByCop(state, dude)) {
				return true;
			}
		}
	}
	return false;
}


bool App::aimedBySwat(const State & state, const Dude & swat) const noexcept
{
	Pos pos = swat.pos;

	while (true) {
		if (map.hasTallWall(pos, swat.dir)) {
			break;
		}

		const Pos nextPos = pos + shiftForDir(swat.dir);

		if (nextPos == state.killer.pos) {
			return true;
		}

		{
			const auto it = map.findPhone(nextPos);
			if (it != map.phones.end()) {
				break;
			}
		}

		{
			const auto it = state.findDude(nextPos);
			if (it != state.dudes.end()) {
				break;
			}
		}

		pos = nextPos;
	}

	return false;
}


bool App::aimedByAnySwat(const State & state) const noexcept
{
	for (const Dude & dude : state.dudes) {
		if (dude.type == Dude::Type::Swat) {
			if (aimedBySwat(state, dude)) return true;
		}
	}
	return false;
}


void App::scare(const State & state, const Pos & pos, Extra & extra) const
{
	for (const Dir dir : kAllDirs) {
		if (map.hasTallWall(pos, dir)) {
			continue;
		}

		const auto it = state.findDude(pos + shiftForDir(dir));
		if (it == state.dudes.end()) {
			// no dude here
			continue;
		}

		const Dude & dude = *it;
		if (dude.type != Dude::Type::Victim && dude.type != Dude::Type::Cat) {
			// can scare only victims and cats
			continue;
		}

		if (dude.type == Dude::Type::Victim) {
			if (!state.light) {
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


void App::call(const State & state, const Dude & who, const Phone & phone, Extra & extra) const
{
	for (const Phone & otherPhone : map.phones) {
		if (otherPhone.pos == phone.pos) continue;
		if (otherPhone.color != phone.color) continue;

		for (const Dir dir : kAllDirs) {
			Pos pos = otherPhone.pos;

			while (true) {
				if (map.hasTallWall(pos, dir)) {
					break;
				}

				const Pos nextPos = pos + shiftForDir(dir);

				const auto it = state.findDude(nextPos);
				if (it != state.dudes.end()) {
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


void App::goDude(State & state, const Dude dude, const Dir dir, const bool called, Extra & extra)
{
	const Res res = go(state, dude.pos, dir, false);

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
		state.moveDude(dude, Dude(target));
		if (res.bump == Bump::Death) {
			extra.killed.push(target);
		}
		if (res.bump == Bump::Drop) {
			const Dude & drop = state.getDude(res.target);
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
					call(state, target, map.getPhone(res.target), extra);
				}
			}
		}
		if (res.bump == Bump::Wall) {
			const Wall & wall = map.getWall(target.pos, dir);
			trySwitchLight(state, wall, dir, extra);
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


void App::kill(State & state, const Dude dude, Extra & extra, std::queue<Extra::Scared> & scared)
{
	{
		const auto it = state.findDude(dude.pos);
		assert(it != state.dudes.end());
		const Dude & dude = *it;
		if (dude.type == Dude::Type::Cat) {
			extra.fail = Extra::Fail::Catality;
		}
		state.dudes.erase(it);
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

	scare(state, dude.pos, extra);
}


void App::processExtra(State & state, Extra & extra)
{
	// check if we stopped in front of a cop
	if (aimedByAnyCop(state)) {
		extra.fail = Extra::Fail::Cop;
		return;
	}

	// check if we are on a line sight of a swat
	if (aimedByAnySwat(state)) {
		extra.fail = Extra::Fail::Swat;
		return;
	}

	if (extra.killed.empty() && extra.dropped.empty() && extra.scared.empty() && extra.called.empty()) {
		return;
	}

	Extra nextExtra;

	while (!extra.killed.empty()) {
		const Dude dude = extra.killed.front();
		extra.killed.pop();
		kill(state, dude, nextExtra, extra.scared);
	}

	while (!extra.dropped.empty()) {
		const Extra::Dropped dropped = extra.dropped.front();
		extra.dropped.pop();

		if (map.hasAnyWall(dropped.drop.pos, dropped.dir)) {
			// cannot drop onto the wall
			continue;
		}

		const Pos dropPos = dropped.drop.pos + shiftForDir(dropped.dir);
		const auto it = state.findDude(dropPos);
		if (it != state.dudes.end()) {
			const Dude & dude = *it;
			if (dude.type == Dude::Type::Drop) {
				// cannot rop onto another drop
				continue;
			}
			kill(state, dude, nextExtra, extra.scared);
		}

		state.moveDude(dropped.drop, Dude {
			.type = Dude::Type::Drop,
			.pos = dropPos,
			.orientation = Orientation::Down,
		});
	}

	while (!extra.scared.empty()) {
		const Extra::Scared scared = extra.scared.front();
		extra.scared.pop();
		goDude(state, scared.dude, scared.dir, false, nextExtra);
	}

	while (!extra.called.empty()) {
		const Extra::Called called = extra.called.front();
		extra.called.pop();
		goDude(state, called.dude, called.dir, true, nextExtra);
	}

	processExtra(state, nextExtra);

	if (nextExtra.fail != Extra::Fail::None) {
		extra.fail = nextExtra.fail;
	}
	if (nextExtra.win != Extra::Win::None) {
		extra.win = nextExtra.win;
	}
}


void App::exec() noexcept
{
	std::unordered_map<State, int> moveIdForState;
	std::vector<Move> moves;
	std::vector<int> winMoveIds;

	const auto moveDistance = [&moves] (const int moveId) -> int {
		int distance = -1;
		for (int id = moveId; id != -1;) {
			id = moves[id].previousId;
			distance++;
		}
		return distance;
	};

	std::queue<int> moveIdsLeft;

	struct MoveRes {
		int id;
		bool same;
	};

// #define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
	static constexpr int kDebugMoveId = 39;
	static constexpr Dir kDebugMoveDir = Dir::Down;
#endif

	const auto addMove = [&moveIdForState, &moves, &moveIdsLeft, &moveDistance](
			Move && move, const bool win) -> MoveRes {
		const auto it = moveIdForState.find(move.state);
		if (it != moveIdForState.end()) {
			const int oldMoveId = it->second;
			const int oldMoveDistance = moveDistance(oldMoveId);
			const int newMoveDistance = moveDistance(move.previousId) + 1;
			if (newMoveDistance < oldMoveDistance) {
				Move & oldMove = moves[oldMoveId];
				oldMove.previousId = move.previousId;
				oldMove.dir = move.dir;
			}
			return MoveRes {
				.id = oldMoveId,
				.same = true,
			};
		} else {
			const int id = moves.size();
			moveIdForState[move.state] = id;
			moves.push_back(std::move(move));
			if (!win) {
				moveIdsLeft.push(id);
			}
			return MoveRes {
				.id = id,
				.same = false,
			};
		}
	};

	addMove(Move {
		.previousId = -1,
		.state = map.state,
	}, false);

#ifdef ENABLE_DEBUG
	const auto getSteps = [&moves] (const int moveId) -> std::vector<int> {
		std::vector<int> steps;
		for (int id = moveId; id != -1;) {
			steps.push_back(id);
			id = moves[id].previousId;
			if (moves[id].previousId == -1) break;
		}
		std::reverse(steps.begin(), steps.end());
		return steps;
	};

	const auto stepsToString = [&moves] (const std::vector<int> & steps) -> std::string {
		std::string s;
		for (const int id : steps) {
			const Dir dir = moves[id].dir;
			s += shortNameForDir(dir);
		}
		return s;
	};
#endif

	while (!moveIdsLeft.empty()) {
		const int currentMoveId = moveIdsLeft.front();
		moveIdsLeft.pop();

		for (const Dir dir : kAllDirs) {
#ifdef ENABLE_DEBUG
		if (currentMoveId == kDebugMoveId && dir == kDebugMoveDir) {
			int b = 1;
		}
#endif

			const Move & currentMove = moves[currentMoveId];
			const State & currentState = currentMove.state;

			State state = currentState;

			bool fail = false;
			bool win = false;

			const Res res = go(state, state.killer.pos, dir, !state.hasVictims());

			Extra extra;

			for (const Teleport & teleport : res.teleports) {
				if (teleport.pos == Pos::null()) break;
				if (teleport.pos != res.pos) {
					scare(state, teleport.pos, extra);
				}
			}

			switch (res.bump) {
			case Bump::Gum : {
				state.killer.pos = res.pos;
				scare(state, state.killer.pos, extra);
			}
				break;

			case Bump::Wall: {
				const Wall & wall = map.getWall(res.pos, dir);
				trySwitchLight(state, wall, dir, extra);

				state.killer.pos = res.pos;
				scare(state, state.killer.pos, extra);
			}
				break;

			case Bump::Dude: {
				state.killer.pos = res.pos;

				const Dude dude = state.getDude(res.target);

				if (dude.type == Dude::Type::Cop) {
					if (aimedByCop(state, dude)) {
						fail = true;
						break;
					}
				}

				if (dude.type == Dude::Type::Swat) {
					if (state.light) {
						// swat kills you instantly when lights are on
						fail = true;
						break;
					}
				}

				scare(state, state.killer.pos, extra);
				extra.killed.push(dude);
			}
				break;

			case Bump::Drop: {
				state.killer.pos = res.pos;

				const Dude drop = state.getDude(res.target);

				scare(state, state.killer.pos, extra);
				if (dirMatchesOrientation(dir, drop.orientation)) {
					extra.dropped.push(Extra::Dropped {
						.drop = drop,
						.dir = dir,
					});
				}
			}
				break;

			case Bump::Phone: {
				state.killer.pos = res.pos;

				const Phone & phone = map.getPhone(res.target);

				scare(state, state.killer.pos, extra);
				call(state, Dude{.type = Dude::Type::Victim, .pos = state.killer.pos}, phone, extra);
			}
				break;

			case Bump::Death:
				fail = true;
				break;

			case Bump::Portal:
				state.killer.pos = res.pos;
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
				processExtra(state, extra);
			}
				break;

			case Bump::Death:
			case Bump::Portal:
				break;
			}

			if (extra.fail != Extra::Fail::None || fail) {
				continue;
			}

			if (extra.win != Extra::Win::None) {
				win = true;
			}

			if (!win) {
				if (!state.hasVictims()) {
					if (map.portal.pos == Pos::null()) {
						win = true;
					}
				}
			}

			const MoveRes moveRes = addMove(Move {
				.dir = dir,
				.previousId = currentMoveId,
				.state = std::move(state),
			}, win);

			if (win && !moveRes.same) {
				winMoveIds.push_back(moveRes.id);
			}

#ifdef ENABLE_DEBUG
			// static const std::string_view kExpectedSteps = "rd";
			static const std::string_view kExpectedSteps = "urdludluludruldrd";

			printf("move: from: %d to: %s same: %d id: %d\n", currentMoveId, nameForDir(dir).data(), moveRes.same, moveRes.id);
			if (!moveRes.same) {
				const Move & m = moves[moveRes.id];
				printf("    killer: %d %d\n", m.state.killer.pos.x, m.state.killer.pos.y);
				for (const Dude & d : m.state.dudes) {
					printf("    dude: %d %d - %s", d.pos.x, d.pos.y, nameForDudeType(d.type).data());
					switch (d.type) {
					case Dude::Type::Victim:
					case Dude::Type::Cat:
						break;
					case Dude::Type::Cop:
					case Dude::Type::Swat:
						printf(" (%s)", nameForDir(d.dir).data());
						break;
					case Dude::Type::Drop:
						printf(" (%s)", nameForOrientation(d.orientation).data());
						break;
					}
					printf("\n");
				}
			}
			fflush(stdout);

			const Move & m = moves[moveRes.id];
			const std::vector<int> steps = getSteps(moveRes.id);
			const std::string stepsString = stepsToString(steps);
			if (stepsString == kExpectedSteps) {
				int b = 1;
			}
#endif
		}
	}

	std::sort(winMoveIds.begin(), winMoveIds.end(), [&moveDistance] (const int a, const int b) {
		return moveDistance(a) < moveDistance(b);
	});

	static constexpr int kShowStepsVerbosity = 2;
	static constexpr int kShowStepsCount = -1;

	printf("moves: total: %d win: %d\n", int(moves.size()), int(winMoveIds.size()));
	if (kShowStepsVerbosity > 0) {
		for (const int winMoveId : winMoveIds) {
			std::vector<int> seq;
			for (int id = winMoveId; id != -1;) {
				seq.push_back(id);
				id = moves[id].previousId;
				if (moves[id].previousId == -1) break;
			}
			std::reverse(seq.begin(), seq.end());
			printf("win move: %d (steps: %d)\n", winMoveId, moveDistance(winMoveId));
			if (kShowStepsVerbosity > 1) {
				int stepIndex = 0;
				for (const int id : seq) {
					const Dir dir = moves[id].dir;
					if (kShowStepsCount == -1 || stepIndex < kShowStepsCount) {
						printf("    % 4d %s\n", id, nameForDir(dir).data());
					}
					stepIndex++;
				}
			}
		}
	}
}
