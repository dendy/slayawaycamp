
#include "App.hpp"

#include <fstream>

#include <QStringList>




Map App::load(const std::filesystem::path & path)
{
	int maxLength = 0;
	std::vector<QString> lines;
	std::ifstream file(path);
	assert(file.good());
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || std::string_view(line).substr(0, 1) == "#") {
			continue;
		}
		const QString qline = QString::fromStdString(line);
		maxLength = std::max(maxLength, qline.length());
		lines.push_back(std::move(qline));
	}
	assert(maxLength > 3);
	assert(lines.size() > 2);

	assert((maxLength - 1) % 3 == 0);
	const int width = (maxLength - 1) / 3;

	assert((lines.size() - 1) % 2 == 0);
	const int height = (lines.size() - 1) / 2;

	Killer killer {
		.pos = Pos::null(),
	};

	Portal portal {
		.pos = Pos::null(),
	};

	std::vector<Dude> dudes;
	std::vector<Mine> mines;
	std::vector<Wall> hwalls;
	std::vector<Wall> vwalls;
	std::vector<Trap> traps;
	std::vector<Phone> phones;

	const auto getDir = [] (const QChar & s) -> Dir {
		if (s == "l") return Dir::Left;
		if (s == "r") return Dir::Right;
		if (s == "u") return Dir::Up;
		if (s == "d") return Dir::Down;
		assert(false);
	};

	const auto getOrientation = [] (const QChar & s) -> Orientation {
		if (s == "h") return Orientation::Horz;
		if (s == "v") return Orientation::Vert;
		assert(false);
	};

	const auto getColor = [] (const QChar & s) -> Color {
		if (s == "b") return Color::Blue;
		if (s == "r") return Color::Red;
		if (s == "y") return Color::Yellow;
		assert(false);
	};

	static const QString kKK = QString::fromUtf8("KK");
	static const QString kPP = QString::fromUtf8("PP");
	static const QString kKP = QString::fromUtf8("KP");
	static const QStringList kTraps = {
		QString::fromUtf8("TT"),
		QString::fromUtf8("xx"),
	};
	static const QString kVV = QString::fromUtf8("VV");
	static const QString kCat = QString::fromUtf8("^^");
	static const QString kMine = QString::fromUtf8("<>");

	static const QStringList kHorzWalls = {
		QString::fromUtf8("=="),
		QString::fromUtf8("══"),
	};
	static const QStringList kVertWalls = {
		QString::fromUtf8("I"),
		QString::fromUtf8("║"),
	};

	static const QStringList kHorzShortWalls = {
		QString::fromUtf8("--"),
		QString::fromUtf8("──"),
	};
	static const QStringList kVertShortWalls = {
		QString::fromUtf8("}"),
		QString::fromUtf8("│"),
	};

	static const QString kHorzEscapeWall = QString::fromUtf8("ee");
	static const QString kVertEscapeWall = QString::fromUtf8("e");

	static const QString kHorzSwitchWall = QString::fromUtf8(">>");
	static const QString kVertSwitchWall = QString::fromUtf8(">");

	static const QString kHorzWinWall = QString::fromUtf8("!!");
	static const QString kVertWinWall = QString::fromUtf8("!");

	static const QString kHorzZapWall = QString::fromUtf8("zz");
	static const QString kVertZapWall = QString::fromUtf8("z");

	static const QStringList kTileBlocks = {
		QString::fromUtf8("██"),
		QString::fromUtf8("▒▒"),
		QString::fromUtf8("░░"),
		QString::fromUtf8(".."),
	};
	static const QStringList kHorzWallBlocks = {
		QString::fromUtf8("██"),
		QString::fromUtf8("▒▒"),
		QString::fromUtf8("░░"),
		QString::fromUtf8("  "),
	};
	static const QStringList kVertWallBlocks = {
		QString::fromUtf8("█"),
		QString::fromUtf8("▒"),
		QString::fromUtf8("░"),
		QString::fromUtf8(" "),
	};
\
	for (int y = 0; y < height; ++y) {
		const QStringView line = lines[y * 2 + 1];
		for (int x = 0; x < width; ++x) {
			const QStringView tile = line.mid(x * 3 + 1, 2);
			if (kTileBlocks.contains(tile)) {
				// empty tile
			} else if (tile == kKK) {
				assert(killer.pos == Pos::null());
				killer.pos = Pos{x, y};
			} else if (tile == kPP) {
				assert(portal.pos == Pos::null());
				portal.pos = Pos{x, y};
			} else if (tile == kKP) {
				assert(killer.pos == Pos::null());
				killer.pos = Pos{x, y};
				assert(portal.pos == Pos::null());
				portal.pos = Pos{x, y};
			} else if (kTraps.contains(tile)) {
				traps.push_back(Trap{Pos{x, y}});
			} else if (tile == kVV) {
				dudes.push_back(Dude {
					.type = Dude::Type::Victim,
					.pos = Pos{x, y},
				});
			} else if (tile == kCat) {
				dudes.push_back(Dude {
					.type = Dude::Type::Cat,
					.pos = Pos{x, y},
				});
			} else if (tile == kMine) {
				mines.push_back(Mine {
					.pos = Pos{x, y},
				});
			} else if (tile.at(0) == 'C') {
				dudes.push_back(Dude {
					.type = Dude::Type::Cop,
					.pos = Pos{x, y},
					.dir = getDir(tile.at(1)),
				});
			} else if (tile.at(0) == 'S') {
				dudes.push_back(Dude {
					.type = Dude::Type::Swat,
					.pos = Pos{x, y},
					.dir = getDir(tile.at(1)),
				});
			} else if (tile.at(0) == "D") {
				dudes.push_back(Dude {
					.type = Dude::Type::Drop,
					.pos = Pos{x, y},
					.orientation = getOrientation(tile.at(1)),
				});
			} else if (tile.at(0) == "P") {
				phones.push_back(Phone {
					.pos = Pos{x, y},
					.color = getColor(tile.at(1)),
				});
			} else {
				assert(false);
			}
		}
	}

	assert(killer.pos != Pos::null());

	for (int y = 0; y < height + 1; ++y) {
		for (int x = 0; x < width; ++x) {
			const QStringView wall = QStringView(lines[y * 2]).mid(x * 3 + 1, 2);
			if (kHorzWallBlocks.contains(wall)) {
				// no wall
			} else if (kHorzWalls.contains(wall)) {
				hwalls.push_back(Wall {
					.type = Wall::Type::Normal,
					.pos = Pos{x, y},
				});
			} else if (kHorzShortWalls.contains(wall)) {
				hwalls.push_back(Wall {
					.type = Wall::Type::Short,
					.pos = Pos{x, y},
				});
			} else if (wall == kHorzEscapeWall) {
				hwalls.push_back(Wall {
					.type = Wall::Type::Escape,
					.pos = Pos{x, y},
				});
			} else if (wall == kHorzSwitchWall) {
				hwalls.push_back(Wall {
					.type = Wall::Type::Switch,
					.pos = Pos{x, y},
				});
			} else if (wall == kHorzWinWall) {
				hwalls.push_back(Wall {
					.type = Wall::Type::Switch,
					.pos = Pos{x, y},
					.win = true,
				});
			} else if (wall == kHorzZapWall) {
				hwalls.push_back(Wall {
					.type = Wall::Type::Zap,
					.pos = Pos{x, y},
				});
			} else {
				fprintf(stderr, "Map hwall error: %d x %d\n", x, y);
				assert(false);
			}
		}
	}

	for (int x = 0; x < width + 1; ++x) {
		for (int y = 0; y < height; ++y) {
			const QStringView wall = QStringView(lines[y * 2 + 1]).mid(x * 3, 1);
			if (kVertWallBlocks.contains(wall)) {
				// no wall
			} else if (kVertWalls.contains(wall)) {
				vwalls.push_back(Wall {
					.type = Wall::Type::Normal,
					.pos = Pos{x, y},
				});
			} else if (kVertShortWalls.contains(wall)) {
				vwalls.push_back(Wall {
					.type = Wall::Type::Short,
					.pos = Pos{x, y},
				});
			} else if (wall == kVertEscapeWall) {
				vwalls.push_back(Wall {
					.type = Wall::Type::Escape,
					.pos = Pos{x, y},
				});
			} else if (wall == kVertSwitchWall) {
				vwalls.push_back(Wall {
					.type = Wall::Type::Switch,
					.pos = Pos{x, y},
				});
			} else if (wall == kVertWinWall) {
				vwalls.push_back(Wall {
					.type = Wall::Type::Switch,
					.pos = Pos{x, y},
					.win = true,
				});
			} else if (wall == kVertZapWall) {
				vwalls.push_back(Wall {
					.type = Wall::Type::Zap,
					.pos = Pos{x, y},
				});
			} else {
				assert(false);
			}
		}
	}

	std::sort(dudes.begin(), dudes.end());
	std::sort(mines.begin(), mines.end());

	return Map {
		.width = width,
		.height = height,
		.hwalls = std::move(hwalls),
		.vwalls = std::move(vwalls),
		.traps = std::move(traps),
		.phones = std::move(phones),
		.portal = std::move(portal),
		.state = State {
			.killer= std::move(killer),
			.dudes = std::move(dudes),
			.mines = std::move(mines),
		},
	};
}


App::App(const std::filesystem::path & path) :
	map(load(path))
{
	exec();
}


const std::initializer_list<Dir> & App::allDirs() noexcept
{
	static const auto list = {Dir::Left, Dir::Right, Dir::Up, Dir::Down};
	return list;
}


const Wall * App::findWall(const Pos & pos, const Dir dir) const noexcept
{
	const Pos shift = shiftForDir(dir);
	const std::vector<Wall> & walls = shift.x != 0 ? map.vwalls : map.hwalls;
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


const Wall & App::getWall(const Pos & pos, const Dir dir) const noexcept
{
	const Wall * const pwall = findWall(pos, dir);
	assert(pwall);
	return *pwall;
}


bool App::hasAnyWall(const Pos & pos, const Dir dir) const noexcept
{
	const Wall * const pwall = findWall(pos, dir);
	return pwall != nullptr;
}


bool App::hasTallWall(const Pos & pos, const Dir dir) const noexcept
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
	const Pos shift = shiftForDir(dir);

	Pos pos = fromPos;

	while (true) {
		if (hasAnyWall(pos, dir)) {
			const Wall & wall = getWall(pos, dir);
			if (state.light && wall.type == Wall::Type::Zap) {
				// bumped into electric wire wall
				return Res {
					.bump = Bump::Death,
					.pos = pos,
				};
			}

			// bumped into a wall
			return Res {
				.bump = Bump::Wall,
				.pos = pos,
			};
		}

		const Pos nextPos = pos + shift;

		{
			const auto it = state.findDude(nextPos);
			if (it != state.dudes.end()) {
				// bumped into dude
				const Dude & dude = *it;

				if (dude.type == Dude::Type::Drop) {
					return Res {
						.bump = Bump::Drop,
						.pos = pos,
						.target = nextPos,
					};
				} else {
					return Res {
						.bump = Bump::Dude,
						.pos = pos,
						.target = nextPos,
					};
				}
			}
		}

		{
			const auto it = map.findPhone(nextPos);
			if (it != map.phones.end()) {
				return Res {
					.bump = Bump::Phone,
					.pos = pos,
					.target = nextPos,
				};
			}
		}

		pos = nextPos;

		{
			const Trap trap = Trap{nextPos};
			const auto it = std::find(map.traps.begin(), map.traps.end(), trap);
			if (it != map.traps.end()) {
				// got into a trap
				return Res {
					.bump = Bump::Death,
					.pos = pos,
				};
			}
		}

		{
			const Mine mine = Mine{nextPos};
			const auto it = std::find(state.mines.begin(), state.mines.end(), mine);
			if (it != state.mines.end()) {
				// steppen on a mine
				state.mines.erase(it);
				return Res {
					.bump = Bump::Death,
					.pos = pos,
				};
			}
		}

		if (portal && pos == map.portal.pos) {
			return Res {
				.bump = Bump::Portal,
				.pos = pos,
			};
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
	if (hasAnyWall(cop.pos, cop.dir)) {
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
		if (hasTallWall(pos, swat.dir)) {
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
	for (const Dir dir : allDirs()) {
		if (hasTallWall(pos, dir)) {
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

		for (const Dir dir : allDirs()) {
			Pos pos = otherPhone.pos;

			while (true) {
				if (hasTallWall(pos, dir)) {
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
	case Bump::Phone: {
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
			const Wall & wall = getWall(target.pos, dir);
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

		if (hasAnyWall(dropped.drop.pos, dropped.dir)) {
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

		for (const Dir dir : allDirs()) {
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

			switch (res.bump) {
			case Bump::Wall: {
				const Wall & wall = getWall(res.pos, dir);
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
			case Bump::Phone: {
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
			static const std::string_view kExpectedSteps = "rdlurdlr";
			// static const std::string_view kExpectedSteps = "rdlurdlrulurdl";

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

	printf("moves: total: %d win: %d\n", int(moves.size()), int(winMoveIds.size()));
	for (const int winMoveId : winMoveIds) {
		std::vector<int> seq;
		for (int id = winMoveId; id != -1;) {
			seq.push_back(id);
			id = moves[id].previousId;
			if (moves[id].previousId == -1) break;
		}
		std::reverse(seq.begin(), seq.end());
		printf("win move: %d (steps: %d)\n", winMoveId, moveDistance(winMoveId));
		for (const int id : seq) {
			const Dir dir = moves[id].dir;
			printf("    % 4d %s\n", id, nameForDir(dir).data());
		}
	}
}
