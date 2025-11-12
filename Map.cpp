
#include "Map.hpp"

#include <charconv>
#include <fstream>

#include <QStringList>




static constexpr int kMaxMapSize = 16;




static const QString kEmptyTile    = QString::fromUtf8("..");

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

static const QString kHorzWinWall = QString::fromUtf8("!!");
static const QString kVertWinWall = QString::fromUtf8("!");

static const QString kHorzEscapeWall = QString::fromUtf8("ee");
static const QString kVertEscapeWall = QString::fromUtf8("e");

static const QString kHorzSwitchWall = QString::fromUtf8(">>");
static const QString kVertSwitchWall = QString::fromUtf8(">");

static const QString kHorzZapWall = QString::fromUtf8("zz");
static const QString kVertZapWall = QString::fromUtf8("z");




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
		QString::fromUtf8("░░"),
		kEmptyTile,
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

	// draw to stdout
	std::for_each(lines.begin(), lines.end(), [&lines] (QString & line) {
		const QByteArray utf8 = line.toUtf8();
		printf("%s\n", utf8.data());
	});
}
