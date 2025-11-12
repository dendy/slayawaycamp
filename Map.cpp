
#include "Map.hpp"

#include <charconv>
#include <fstream>
#include <map>
#include <queue>

#include <QStringList>




static constexpr int kMaxMapSize = 16;




static const QString kEmptyTile    = QString::fromUtf8("..");
static const QString kBlockTile    = QString::fromUtf8("░░");

static const QString kKiller       = QString::fromUtf8("KK");
static const QString kPortal       = QString::fromUtf8("PP");
static const QString kKillerPortal = QString::fromUtf8("KP");
static const QString kKillerGum    = QString::fromUtf8("Kg");

static const QString kVictim       = QString::fromUtf8("VV");
static const QString kVictimGum    = QString::fromUtf8("Vg");

static const QString kCat          = QString::fromUtf8("^^");
static const QString kMine         = QString::fromUtf8("<>");
static const QString kGum          = QString::fromUtf8("gg");

static const QString kCop          = QString::fromUtf8("C");
static const QString kSwat         = QString::fromUtf8("S");
static const QString kDrop         = QString::fromUtf8("D");

static const QString kBlockHorzWall = QString::fromUtf8("░░");
static const QString kBlockVertWall = QString::fromUtf8("░");

static const QStringList kHorzWalls = {
	QString::fromUtf8("══"),
	QString::fromUtf8("=="),
};

static const QStringList kVertWalls = {
	QString::fromUtf8("║"),
	QString::fromUtf8("I"),
};

static const QStringList kHorzShortWalls = {
	QString::fromUtf8("──"),
	QString::fromUtf8("--"),
};
static const QStringList kVertShortWalls = {
	QString::fromUtf8("│"),
	QString::fromUtf8("}"),
};

static const QString kHorzWinWall    = QString::fromUtf8("!!");
static const QString kVertWinWall    = QString::fromUtf8("!");
static const QString kHorzEscapeWall = QString::fromUtf8("ee");
static const QString kVertEscapeWall = QString::fromUtf8("e");
static const QString kHorzSwitchWall = QString::fromUtf8(">>");
static const QString kVertSwitchWall = QString::fromUtf8(">");
static const QString kHorzZapWall    = QString::fromUtf8("zz");
static const QString kVertZapWall    = QString::fromUtf8("z");

static const QString kCornerNormalLeftRightDown = QString::fromUtf8("╦");
static const QString kCornerNormalLeftRightUp   = QString::fromUtf8("╩");
static const QString kCornerNormalUpDownLeft    = QString::fromUtf8("╣");
static const QString kCornerNormalUpDownRight   = QString::fromUtf8("╠");
static const QString kCornerNormalCross         = QString::fromUtf8("╬");
static const QString kCornerNormalDownLeft      = QString::fromUtf8("╗");
static const QString kCornerNormalDownRight     = QString::fromUtf8("╔");
static const QString kCornerNormalUpLeft        = QString::fromUtf8("╝");
static const QString kCornerNormalUpRight       = QString::fromUtf8("╚");
static const QString kCornerNormalLeftRight     = QString::fromUtf8("═");
static const QString kCornerNormalUpDown        = QString::fromUtf8("║");

static const QString kCornerBlock = QString::fromUtf8("░");




static Dir dirFromString(const QChar & s) noexcept
{
	if (s == 'l') return Dir::Left;
	if (s == 'r') return Dir::Right;
	if (s == 'u') return Dir::Up;
	if (s == 'd') return Dir::Down;
	assert(false);
}


static QChar dirToString(const Dir & dir) noexcept
{
	switch (dir) {
	case Dir::Left:  return 'l';
	case Dir::Right: return 'r';
	case Dir::Up:    return 'u';
	case Dir::Down:  return 'd';
	}
	assert(false);
}


static Orientation orientationFromString(const QChar & s) noexcept
{
	if (s == 'h') return Orientation::Horz;
	if (s == 'v') return Orientation::Vert;
	assert(false);
}


static QChar orientationToString(const Orientation & orientation) noexcept
{
	switch (orientation) {
	case Orientation::Horz: return 'h';
	case Orientation::Vert: return 'v';
	case Orientation::Down: return 'd';
	}
	assert(false);
}


static Color colorFromString(const QChar & s) noexcept
{
	if (s == 'b') return Color::Blue;
	if (s == 'r') return Color::Red;
	if (s == 'y') return Color::Yellow;
	assert(false);
}


static QChar colorToString(const Color & color) noexcept
{
	switch (color) {
	case Color::Blue:   return 'b';
	case Color::Red:    return 'r';
	case Color::Yellow: return 'y';
	}
	assert(false);
}




Map Map::load(const std::filesystem::path & path)
{
	static constexpr std::string_view kNamePrefix = "name: ";
	static constexpr std::string_view kTurnsPrefix = "turns: ";

	printf("Loading: %s\n", path.filename().c_str());
	fflush(stdout);

	std::string name;
	int turns = -1;

	int maxLength = 0;
	std::vector<QString> lines;
	std::ifstream file(path);
	assert(file.good());
	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || std::string_view(line).substr(0, 1) == "#") {
			continue;
		}
		if (line.starts_with(kNamePrefix)) {
			assert(name.empty());
			name = line.substr(kNamePrefix.size());
			printf("Level name: %s\n", name.c_str());
			fflush(stdout);
			continue;
		}
		if (line.starts_with(kTurnsPrefix)) {
			assert(turns == -1);
			const std::from_chars_result r = std::from_chars(
					line.data() + kTurnsPrefix.size(), line.data() + line.size(), turns);
			assert(!std::make_error_condition(r.ec) && turns > 0);
			printf("Level turns: %d\n", turns);
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
	assert(width <= kMaxMapSize);

	assert((lines.size() - 1) % 2 == 0);
	const int height = (lines.size() - 1) / 2;
	assert(height <= kMaxMapSize);

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
	std::vector<Gum> gums;
	std::vector<Teleport> teleports;

	static const QStringList kTraps = {
		QString::fromUtf8("TT"),
		QString::fromUtf8("xx"),
	};

	static const QStringList kTileBlocks = {
		QString::fromUtf8("██"),
		QString::fromUtf8("▒▒"),
		kBlockTile,
		kEmptyTile,
	};
	static const QStringList kHorzWallBlocks = {
		QString::fromUtf8("██"),
		QString::fromUtf8("▒▒"),
		kBlockHorzWall,
		QString::fromUtf8("  "),
	};
	static const QStringList kVertWallBlocks = {
		QString::fromUtf8("█"),
		QString::fromUtf8("▒"),
		kBlockVertWall,
		QString::fromUtf8(" "),
	};
\
	for (int y = 0; y < height; ++y) {
		const QStringView line = lines[y * 2 + 1];
		for (int x = 0; x < width; ++x) {
			const QStringView tile = line.mid(x * 3 + 1, 2);
			if (kTileBlocks.contains(tile)) {
				// empty tile
			} else if (tile == kKiller) {
				assert(killer.pos == Pos::null());
				killer.pos = Pos{x, y};
			} else if (tile == kPortal) {
				assert(portal.pos == Pos::null());
				portal.pos = Pos{x, y};
			} else if (tile == kKillerPortal) {
				assert(killer.pos == Pos::null());
				killer.pos = Pos{x, y};
				assert(portal.pos == Pos::null());
				portal.pos = Pos{x, y};
			} else if (tile == kKillerGum) {
				assert(killer.pos == Pos::null());
				killer.pos = Pos{x, y};
				gums.push_back(Gum {
					.pos = Pos{x, y},
				});
			} else if (tile == kVictim) {
				dudes.push_back(Dude {
					.type = Dude::Type::Victim,
					.pos = Pos{x, y},
				});
			} else if (tile == kVictimGum) {
				dudes.push_back(Dude {
					.type = Dude::Type::Victim,
					.pos = Pos{x, y},
				});
				gums.push_back(Gum {
					.pos = Pos{x, y},
				});
			} else if (kTraps.contains(tile)) {
				traps.push_back(Trap{Pos{x, y}});
			} else if (tile == kCat) {
				dudes.push_back(Dude {
					.type = Dude::Type::Cat,
					.pos = Pos{x, y},
				});
			} else if (tile == kMine) {
				mines.push_back(Mine {
					.pos = Pos{x, y},
				});
			} else if (tile == kGum) {
				gums.push_back(Gum {
					.pos = Pos{x, y},
				});
			} else if (tile.at(0) == kCop) {
				dudes.push_back(Dude {
					.type = Dude::Type::Cop,
					.pos = Pos{x, y},
					.dir = dirFromString(tile.at(1)),
				});
			} else if (tile.at(0) == kSwat) {
				dudes.push_back(Dude {
					.type = Dude::Type::Swat,
					.pos = Pos{x, y},
					.dir = dirFromString(tile.at(1)),
				});
			} else if (tile.at(0) == kDrop) {
				dudes.push_back(Dude {
					.type = Dude::Type::Drop,
					.pos = Pos{x, y},
					.orientation = orientationFromString(tile.at(1)),
				});
			} else if (tile.at(0) == 'P') {
				phones.push_back(Phone {
					.pos = Pos{x, y},
					.color = colorFromString(tile.at(1)),
				});
			} else if (tile.at(0) == 'T') {
				teleports.push_back(Teleport {
					.pos = Pos{x, y},
					.color = colorFromString(tile.at(1)),
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

	for (const Color color : kAllColors) {
		{
			const int count = std::count_if(phones.begin(), phones.end(),
					[color] (const Phone & phone) {
						return phone.color == color;
					});
			assert(count == 0 || count == 2);
		}
		{
			const int count = std::count_if(teleports.begin(), teleports.end(),
					[color] (const Teleport & teleport) {
						return teleport.color == color;
					});
			assert(count == 0 || count == 2);
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
		.gums = std::move(gums),
		.teleports = std::move(teleports),
		.portal = std::move(portal),
		.state = State {
			.killer= std::move(killer),
			.dudes = std::move(dudes),
			.mines = std::move(mines),
		},
	};
}


void Map::draw(const Map & map)
{
	std::vector<QString> lines;
	lines.resize(map.height * 2 + 1);
	std::for_each(lines.begin(), lines.end(), [&map] (QString & line) {
		line.resize(map.width * 3 + 1, QChar(' '));
	});

	const auto tile = [&lines, &map] (const Pos & pos) -> QChar * {
		assert(pos.x >= 0 && pos.x < map.width);
		assert(pos.y >= 0 && pos.y < map.height);
		return lines[pos.y * 2 + 1].data() + pos.x * 3 + 1;
	};

	const auto hwall = [&lines, &map] (const Pos & pos) -> QChar * {
		assert(pos.x >= 0 && pos.x < map.width);
		assert(pos.y >= 0 && pos.y < map.height + 1);
		return lines[pos.y * 2].data() + pos.x * 3 + 1;
	};

	const auto vwall = [&lines, &map] (const Pos & pos) -> QChar * {
		assert(pos.x >= 0 && pos.x < map.width + 1);
		assert(pos.y >= 0 && pos.y < map.height);
		return lines[pos.y * 2 + 1].data() + pos.x * 3;
	};

	const auto corner = [&lines, &map] (const Pos & pos) -> QChar * {
		assert(pos.x >= 0 && pos.x < map.width + 1);
		assert(pos.y >= 0 && pos.y < map.height + 1);
		return lines[pos.y * 2].data() + pos.x * 3;
	};

	const auto setTile = [&tile] (const Pos & pos, const QStringView & value) {
		QChar * const s = tile(pos);
		s[0] = value[0];
		s[1] = value[1];
	};

	const auto setHorzWall = [&hwall] (const Pos & pos, const QStringView & value) {
		QChar * const s = hwall(pos);
		s[0] = value[0];
		s[1] = value[1];
	};

	const auto setVertWall = [&vwall] (const Pos & pos, const QStringView & value) {
		QChar * const s = vwall(pos);
		s[0] = value[0];
	};

	for (int y = 0; y < map.height; ++y) {
		for (int x = 0; x < map.width; ++x) {
			setTile(Pos{x, y}, kEmptyTile);
		}
	}

	setTile(map.state.killer.pos, [&map] {
		if (map.portal.pos == map.state.killer.pos) {
			return kKillerPortal;
		}
		if (map.hasGum(map.state.killer.pos)) {
			return kKillerGum;
		}
		return kKiller;
	}());

	if (map.portal.pos != map.state.killer.pos) {
		setTile(map.portal.pos, kPortal);
	}

	QString tmpTile;
	tmpTile.resize(2);

	std::for_each(map.state.dudes.begin(), map.state.dudes.end(),
			[&setTile, &map, &tmpTile] (const Dude & dude) {
				setTile(dude.pos, [&dude, &map, &tmpTile] {
					const bool hasGum = map.hasGum(dude.pos);
					switch (dude.type) {
					case Dude::Type::Victim:
						return hasGum ? kVictimGum : kVictim;
					case Dude::Type::Cat:
						assert(!hasGum);
						return kCat;
					case Dude::Type::Cop:
						tmpTile[0] = kCop[0];
						tmpTile[1] = dirToString(dude.dir);
						return tmpTile;
					case Dude::Type::Swat:
						tmpTile[0] = kSwat[0];
						tmpTile[1] = dirToString(dude.dir);
						return tmpTile;
					case Dude::Type::Drop:
						tmpTile[0] = kDrop[0];
						tmpTile[1] = orientationToString(dude.orientation);
						return tmpTile;
					}
					assert(false);
				}());
			});

	std::for_each(map.state.mines.begin(), map.state.mines.end(),
			[&setTile, &map] (const Mine & mine) {
				setTile(mine.pos, kMine);
			});

	std::for_each(map.hwalls.begin(), map.hwalls.end(),
			[&setHorzWall] (const Wall & wall) {
				setHorzWall(wall.pos, [&wall] {
					if (wall.win) {
						return kHorzWinWall;
					}
					switch (wall.type) {
					case Wall::Type::Normal:
						return kHorzWalls[0];
					case Wall::Type::Escape:
						return kHorzEscapeWall;
					case Wall::Type::Switch:
						return kHorzSwitchWall;
					case Wall::Type::Short:
						return kHorzShortWalls[0];
					case Wall::Type::Zap:
						return kHorzZapWall;
					}
					assert(false);
				}());
			});

	std::for_each(map.vwalls.begin(), map.vwalls.end(),
			[&setVertWall] (const Wall & wall) {
				setVertWall(wall.pos, [&wall] {
					if (wall.win) {
						return kVertWinWall;
					}
					switch (wall.type) {
					case Wall::Type::Normal:
						return kVertWalls[0];
					case Wall::Type::Escape:
						return kVertEscapeWall;
					case Wall::Type::Switch:
						return kVertSwitchWall;
					case Wall::Type::Short:
						return kVertShortWalls[0];
					case Wall::Type::Zap:
						return kVertZapWall;
					}
					assert(false);
				}());
			});

	const auto cornerForTag = [] () -> std::map<std::string_view, QStringView> {
		std::map<std::string_view, QStringView> m;
		// lrud
		m["nn  "] = kCornerNormalLeftRight;
		m["  nn"] = kCornerNormalUpDown;
		m["n n "] = kCornerNormalUpLeft;
		m["n  n"] = kCornerNormalDownLeft;
		m[" nn "] = kCornerNormalUpRight;
		m[" n n"] = kCornerNormalDownRight;
		m["nnn "] = kCornerNormalLeftRightUp;
		m["nn n"] = kCornerNormalLeftRightDown;
		m["n nn"] = kCornerNormalUpDownLeft;
		m[" nnn"] = kCornerNormalUpDownRight;
		m["nnnn"] = kCornerNormalCross;
		return m;
	}();

	const auto makeCorner = [&map, &cornerForTag] (const Pos & pos) -> QStringView {
		const Wall * const rwall = map.findWall(pos, Dir::Up);
		const Wall * const dwall = map.findWall(pos, Dir::Left);
		const Wall * const lwall = map.findWall(pos + Pos{-1, -1}, Dir::Down);
		const Wall * const uwall = map.findWall(pos + Pos{-1, -1}, Dir::Right);
		char tag[4] = {' ', ' ', ' ', ' '};
		if (lwall) {
			if (lwall->type == Wall::Type::Normal) {
				tag[int(Dir::Left)] = 'n';
			}
		}
		if (rwall) {
			if (rwall->type == Wall::Type::Normal) {
				tag[int(Dir::Right)] = 'n';
			}
		}
		if (uwall) {
			if (uwall->type == Wall::Type::Normal) {
				tag[int(Dir::Up)] = 'n';
			}
		}
		if (dwall) {
			if (dwall->type == Wall::Type::Normal) {
				tag[int(Dir::Down)] = 'n';
			}
		}
		const auto it = cornerForTag.find(std::string_view(tag, std::extent_v<decltype(tag)>));
		if (it != cornerForTag.end()) {
			return it->second;
		}
		return {};
	};

	for (int y = 0; y <= map.height; ++y) {
		for (int x = 0; x <= map.width; ++x) {
			QChar * const s = corner(Pos{x, y});
			const QStringView c = makeCorner(Pos{x, y});
			if (!c.empty()) {
				*s = c[0];
			}
		}
	}

	// blocks
	{
		const auto hasSomething = [&map] (const Pos & pos) -> bool {
			if (map.state.killer.pos == pos) return true;
			for (const Dude & dude : map.state.dudes) {
				if (dude.pos == pos) return true;
			}
			for (const Mine & mine : map.state.mines) {
				if (mine.pos == pos) return true;
			}
			for (const Trap & trap : map.traps) {
				if (trap.pos == pos) return true;
			}
			for (const Phone & phone : map.phones) {
				if (phone.pos == pos) return true;
			}
			for (const Gum & gum : map.gums) {
				if (gum.pos == pos) return true;
			}
			for (const Teleport & teleport : map.teleports) {
				if (teleport.pos == pos) return true;
			}
			if (map.portal.pos == pos) return true;
			return false;
		};

		struct Tile {
			bool checked = false;
		};
		std::vector<Tile> checkedTiles(map.width * map.height);
		const auto checkedTile = [&checkedTiles, &map] (const Pos & pos) -> bool & {
			return checkedTiles[pos.y * map.width + pos.x].checked;
		};

		const auto makeArea = [&checkedTile, &map, &hasSomething, &setTile,
				&setHorzWall, &setVertWall, &corner] (const Pos & startPos) {
			bool & checked = checkedTile(startPos);
			if (checked) return;
			checked = true;

			std::vector<Tile> area(map.width * map.height);
			const auto areaTile = [&area, &map] (const Pos & pos) -> bool & {
				return area[pos.y * map.width + pos.x].checked;
			};

			std::queue<Pos> queue;
			queue.push(startPos);

			bool blocked = true;
			int count = 0;

			while (!queue.empty()) {
				const Pos p = queue.front();
				queue.pop();

printf("p: %d %d\n", p.x, p.y);
fflush(stdout);

				assert(!areaTile(p));
				areaTile(p) = true;
				count++;

				for (const Dir & dir : kAllDirs) {
					const Pos dirPos = p + shiftForDir(dir);
					if (map.contains(dirPos)) {
						bool & a = checkedTile(dirPos);
						if (!a) {
							if (!map.hasTallWall(p, dir)) {
								if (hasSomething(dirPos)) {
									blocked = false;
								}
								a = true;
								queue.push(dirPos);
							}
						}
					}
				}
			}

			printf("area: count = %d blocked = %d\n", count, blocked);
			fflush(stdout);

			if (blocked) {
				for (int y = 0; y < map.height; ++y) {
					for (int x = 0; x < map.width; ++x) {
						const Pos pos = Pos{x, y};
						if (areaTile(pos)) {
							setTile(pos, kBlockTile);
							const Pos rightPos = pos + shiftForDir(Dir::Right);
							const Pos downPos = pos + shiftForDir(Dir::Down);
							const Pos downRightPos = downPos + shiftForDir(Dir::Right);
							const bool hasRight = map.contains(rightPos) && areaTile(rightPos);
							const bool hasDown = map.contains(downPos) && areaTile(downPos);
							if (hasRight) {
								setVertWall(rightPos, kBlockVertWall);
							}
							if (hasDown) {
								setHorzWall(downPos, kBlockHorzWall);
							}
							if (hasRight && hasDown) {
								*corner(downRightPos) = kCornerBlock[0];
							}
						}
					}
				}
			}
		};

		for (int y = 0; y < map.height; ++y) {
			for (int x = 0; x < map.width; ++x) {
				makeArea(Pos{x, y});
			}
		}
	}

	// draw to stdout
	std::for_each(lines.begin(), lines.end(), [&lines] (QString & line) {
		const QByteArray utf8 = line.toUtf8();
		printf("%s\n", utf8.data());
	});
}


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
