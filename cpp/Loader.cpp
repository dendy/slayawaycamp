
#include "Loader.hpp"

#include <array>
#include <charconv>
#include <cstring>
#include <fstream>
#include <map>
#include <queue>




static constexpr int kMaxMapSize = 16;




static const QChar uqchar(const char * s) noexcept
{
	const QString qs = QString::fromUtf8(s);
	assert(qs.length() == 1);
	return qs[0];
}




const Loader::BoxProfile Loader::kLightBoxProfile {
	.hasSingle = true,
	.h = uqchar("─"),
	.v = uqchar("│"),
};

const Loader::BoxProfile Loader::kHeavyBoxProfile {
	.hasSingle = true,
	.h = uqchar("━"),
	.v = uqchar("┃"),
};

const Loader::BoxProfile Loader::kDoubleBoxProfile {
	.hasSingle= false,
	.h = uqchar("═"),
	.v = uqchar("║"),
};




static constexpr std::string_view kBoxSymbols[] = {

"   " " │ " "   " "   " " │ " " │ " " │ " " │ " "   " " │ " " │ " "   " "   " " │ " "   ",
"───" " │ " " ┌─" "─┐ " " └─" "─┘ " " ├─" "─┤ " "─┬─" "─┴─" "─┼─" "─╴ " " ╶─" " ╵ " " ╷ ",
"   " " │ " " │ " " │ " "   " "   " " │ " " │ " " │ " "   " " │ " "   " "   " "   " " │ ",

"   " " ┃ " "   " "   " " ┃ " " ┃ " " ┃ " " ┃ " "   " " ┃ " " ┃ " "   " "   " " ┃ " "   ",
"━━━" " ┃ " " ┏━" "━┓ " " ┗━" "━┛ " " ┣━" "━┫ " "━┳━" "━┻━" "━╋━" "━╸ " " ╺━" " ╹ " " ╻ ",
"   " " ┃ " " ┃ " " ┃ " "   " "   " " ┃ " " ┃ " " ┃ " "   " " ┃ " "   " "   " "   " " ┃ ",

"   " "   " " │ " " ┃ ",
"─╼━" "━╾─" " ╽ " " ╿ ",
"   " "   " " ┃ " " │ ",

" ┃ " " ┃ " " │ " " ┃ " " ┃ " " ┃ " " │ " " │ " " │ " " │ " " ┃ " " │ ",
"─╊━" "━╉─" "━╈━" "━╇━" "━╃─" "─╄━" "─╆━" "━╅─" "━┽─" "─┾━" "─╀─" "─╁─",
" ┃ " " ┃ " " ┃ " " │ " " │ " " │ " " ┃ " " ┃ " " │ " " │ " " │ " " ┃ ",

"   " "   " " ┃ " " ┃ " " ┃ " " ┃ " "   " " ┃ " " ┃ ",
" ┎─" "─┒ " " ┖─" "─┚ " " ┠─" "─┨ " "─┰─" "─┸─" "─╂─",
" ┃ " " ┃ " "   " "   " " ┃ " " ┃ " " ┃ " "   " " ┃ ",

"   " "   " " │ " " │ " " │ " " │ " "   " " │ " " │ ",
" ┍━" "━┑ " " ┕━" "━┙ " " ┝━" "━┥ " "━┯━" "━┷━" "━┿━",
" │ " " │ " "   " "   " " │ " " │ " " │ " "   " " │ ",

" ┃ " " ┃ " " │ " " │ " "   " "   " " │ " " │ ",
" ┞─" "─┦ " " ┟─" "─┧ " "━┭─" "─┮━" "━┵─" "─┶━",
" │ " " │ " " ┃ " " ┃ " " │ " " │ " "   " "   ",

" │ " " │ " " ┃ " " ┃ " "   " "   " " ┃ " " ┃ ",
" ┢━" "━┪ " " ┡━" "━┩ " "━┱─" "─┲━" "━┹─" "─┺━",
" ┃ " " ┃ " " │ " " │ " " ┃ " " ┃ " "   " "   ",

"   " " ║ " "   " "   " " ║ " " ║ " " ║ " " ║ " "   " " ║ " " ║ ",
"═══" " ║ " " ╔═" "═╗ " " ╚═" "═╝ " " ╠═" "═╣ " "═╦═" "═╩═" "═╬═",
"   " " ║ " " ║ " " ║ " "   " "   " " ║ " " ║ " " ║ " "   " " ║ ",

"   " "   " " ║ " " ║ " " ║ " " ║ " "   " " ║ " " ║ ",
" ╓─" "─╖ " " ╙─" "─╜ " " ╟─" "─╢ " "─╥─" "─╨─" "─╫─",
" ║ " " ║ " "   " "   " " ║ " " ║ " " ║ " "   " " ║ ",

"   " "   " " │ " " │ " " │ " " │ " "   " " │ " " │ ",
" ╒═" "═╕ " " ╘═" "═╛ " " ╞═" "═╡ " "═╤═" "═╧═" "═╪═",
" │ " " │ " "   " "   " " │ " " │ " " │ " "   " " │ ",
};


static const QString kEmptyTile    = QString::fromUtf8("..");
static const QString kBlockTile    = QString::fromUtf8("░░");

static const QString kKiller       = QString::fromUtf8("KK");
static const QString kPortal       = QString::fromUtf8("PP");
static const QString kKillerPortal = QString::fromUtf8("KP");
static const QString kKillerGum    = QString::fromUtf8("Kg");

static const QString kVictim       = QString::fromUtf8("VV");
static const QString kVictimGum    = QString::fromUtf8("Vg");

static const QString kCat          = QString::fromUtf8("^^");
static const QString kCatPortal    = QString::fromUtf8("^P");
static const QString kMine         = QString::fromUtf8("<>");
static const QString kGum          = QString::fromUtf8("gg");

static const QString kCop          = QString::fromUtf8("C");
static const QString kSwat         = QString::fromUtf8("S");
static const QString kSwatGum      = QString::fromUtf8("W");
static const QString kDrop         = QString::fromUtf8("D");
static const QString kPhone        = QString::fromUtf8("P");
static const QString kTeleport     = QString::fromUtf8("T");

static const std::array<QString, 2> kTraps = {
	QString::fromUtf8("xx"),
	QString::fromUtf8("TT"),
};

static const QString kBlockHorzWall = QString::fromUtf8("░░");
static const QString kBlockVertWall = QString::fromUtf8("░");

static const std::array<QString, 3> kHorzWalls = {
	QString::fromUtf8("━━"),
	QString::fromUtf8("══"),
	QString::fromUtf8("=="),
};

static const std::array<QString, 3> kVertWalls = {
	QString::fromUtf8("┃"),
	QString::fromUtf8("║"),
	QString::fromUtf8("I"),
};

static const std::array<QString, 2> kHorzShortWalls = {
	QString::fromUtf8("──"),
	QString::fromUtf8("--"),
};
static const std::array<QString, 2> kVertShortWalls = {
	QString::fromUtf8("│"),
	QString::fromUtf8("|"),
};

static const std::array<QString, 2> kHorzZapWalls = {
	QString::fromUtf8("⚡︎⚡︎"),
	QString::fromUtf8("zz"),
};
static const std::array<QString, 2> kVertZapWalls = {
	QString::fromUtf8("⚡︎"),
	QString::fromUtf8("z"),
};

static const QString kHorzWinWall    = QString::fromUtf8("!!");
static const QString kVertWinWall    = QString::fromUtf8("!");
static const QString kHorzEscapeWall = QString::fromUtf8("ee");
static const QString kVertEscapeWall = QString::fromUtf8("e");
static const QString kHorzSwitchWall = QString::fromUtf8(">>");
static const QString kVertSwitchWall = QString::fromUtf8(">");

struct CornerBoxSolid {
	QChar leftRight;
	QChar upDown;
	QChar leftRightDown;
	QChar leftRightUp;
	QChar upDownLeft;
	QChar upDownRight;
	QChar cross;
	QChar downLeft;
	QChar downRight;
	QChar upLeft;
	QChar upRight;
	QChar left;
	QChar right;
	QChar up;
	QChar down;
};

struct CornerBoxShift {
	QChar leftRightToDown;
	QChar leftRightToUp;
	QChar upDownToLeft;
	QChar upDownToRight;
	QChar upDownToCross;
	QChar downToLeft;
	QChar downToRight;
	QChar upToLeft;
	QChar upToRight;
};

#if 0
static CornerBoxSolid kDoubleCornerBoxSolid {
	.leftRight     = uqchar("═"),
	.upDown        = uqchar("║"),
	.leftRightDown = uqchar("╦"),
	.leftRightUp   = uqchar("╩"),
	.upDownLeft    = uqchar("╣"),
	.upDownRight   = uqchar("╠"),
	.cross         = uqchar("╬"),
	.downLeft      = uqchar("╗"),
	.downRight     = uqchar("╔"),
	.upLeft        = uqchar("╝"),
	.upRight       = uqchar("╚"),
};

static CornerBoxSolid kLightCornerBoxSolid {
	.leftRight     = uqchar("─"),
	.upDown        = uqchar("│"),
	.leftRightDown = uqchar("┬"),
	.leftRightUp   = uqchar("┴"),
	.upDownLeft    = uqchar("┤"),
	.upDownRight   = uqchar("├"),
	.cross         = uqchar("┼"),
	.downLeft      = uqchar("┐"),
	.downRight     = uqchar("┌"),
	.upLeft        = uqchar("┘"),
	.upRight       = uqchar("└"),
	.left          = uqchar("╴"),
	.right         = uqchar("╶"),
	.up            = uqchar("╵"),
	.down          = uqchar("╷"),
};

static CornerBoxSolid kHeavyCornerBoxSolid {
	.leftRight     = uqchar("─"),
	.upDown        = uqchar("│"),
	.leftRightDown = uqchar("┬"),
	.leftRightUp   = uqchar("┴"),
	.upDownLeft    = uqchar("┤"),
	.upDownRight   = uqchar("├"),
	.cross         = uqchar("┼"),
	.downLeft      = uqchar("┐"),
	.downRight     = uqchar("┌"),
	.upLeft        = uqchar("┘"),
	.upRight       = uqchar("└"),
	.left          = uqchar("╴"),
	.right         = uqchar("╶"),
	.up            = uqchar("╵"),
	.down          = uqchar("╷"),
};

static CornerBoxShift kDoubleToLightBoxShift {
	.leftRightToDown = uqchar("╤"),
	.leftRightToUp =   uqchar("╧"),
	.upDownToLeft    = uqchar("╢"),
	.upDownToRight   = uqchar("╟"),
	.upDownToCross   = uqchar("╫"),
	.downToLeft      = uqchar("╖"),
	.downToRight     = uqchar("╓"),
	.upToLeft        = uqchar("╜"),
	.upToRight       = uqchar("╙"),
};

static CornerBoxShift kLightToDoubleBoxShift {
	.leftRightToDown = uqchar("╥"),
	.leftRightToUp =   uqchar("╨"),
	.upDownToLeft    = uqchar("╡"),
	.upDownToRight   = uqchar("╞"),
	.upDownToCross   = uqchar("╪"),
	.downToLeft      = uqchar("╕"),
	.downToRight     = uqchar("╒"),
	.upToLeft        = uqchar("╛"),
	.upToRight       = uqchar("╘"),
};
#endif

static const QString kCornerNormalLeftRight     = QString::fromUtf8("═");
static const QString kCornerNormalUpDown        = QString::fromUtf8("║");
static const QString kCornerNormalLeftRightDown = QString::fromUtf8("╦");
static const QString kCornerNormalLeftRightUp   = QString::fromUtf8("╩");
static const QString kCornerNormalUpDownLeft    = QString::fromUtf8("╣");
static const QString kCornerNormalUpDownRight   = QString::fromUtf8("╠");
static const QString kCornerNormalCross         = QString::fromUtf8("╬");
static const QString kCornerNormalDownLeft      = QString::fromUtf8("╗");
static const QString kCornerNormalDownRight     = QString::fromUtf8("╔");
static const QString kCornerNormalUpLeft        = QString::fromUtf8("╝");
static const QString kCornerNormalUpRight       = QString::fromUtf8("╚");

static const QString kCornerShortLeftRight     = QString::fromUtf8("─");
static const QString kCornerShortUpDown        = QString::fromUtf8("│");
static const QString kCornerShortLeftRightDown = QString::fromUtf8("┬");
static const QString kCornerShortLeftRightUp   = QString::fromUtf8("┴");
static const QString kCornerShortUpDownLeft    = QString::fromUtf8("┤");
static const QString kCornerShortUpDownRight   = QString::fromUtf8("├");
static const QString kCornerShortCross         = QString::fromUtf8("┼");
static const QString kCornerShortDownLeft      = QString::fromUtf8("┐");
static const QString kCornerShortDownRight     = QString::fromUtf8("┌");
static const QString kCornerShortUpLeft        = QString::fromUtf8("┘");
static const QString kCornerShortUpRight       = QString::fromUtf8("└");

static const QString kCornerNormalLeftRightShortDown = QString::fromUtf8("╤");
static const QString kCornerNormalLeftRightShortUp   = QString::fromUtf8("╧");
static const QString kCornerNormalUpDownShortLeft    = QString::fromUtf8("╢");
static const QString kCornerNormalUpDownShortRight   = QString::fromUtf8("╟");
static const QString kCornerNormalUpDownShortCross   = QString::fromUtf8("╫");
static const QString kCornerNormalDownShortLeft      = QString::fromUtf8("╖");
static const QString kCornerNormalDownShortRight     = QString::fromUtf8("╓");
static const QString kCornerNormalUpShortLeft        = QString::fromUtf8("╜");
static const QString kCornerNormalUpShortRight       = QString::fromUtf8("╙");

static const QString kCornerShortLeftRightNormalDown = QString::fromUtf8("╥");
static const QString kCornerShortLeftRightNormalUp   = QString::fromUtf8("╨");
static const QString kCornerShortUpDownNormalLeft    = QString::fromUtf8("╡");
static const QString kCornerShortUpDownNormalRight   = QString::fromUtf8("╞");
static const QString kCornerShortUpDownNormalCross   = QString::fromUtf8("╪");
static const QString kCornerShortDownNormalLeft      = QString::fromUtf8("╕");
static const QString kCornerShortDownNormalRight     = QString::fromUtf8("╒");
static const QString kCornerShortUpNormalLeft        = QString::fromUtf8("╛");
static const QString kCornerShortUpNormalRight       = QString::fromUtf8("╘");

static const QString kCornerBlock = QString::fromUtf8("░");
static const QString kCornerZap   = QString::fromUtf8("z");




template <size_t N>
static bool listContains(const std::array<QString, N> & list, const QStringView & sv) noexcept
{
	return std::find(list.begin(), list.end(), sv) != list.end();
}


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




QChar Loader::_profileCharForDir(const Loader::BoxProfile & profile, const Dir dir) noexcept
{
	switch (dir) {
	case Dir::Left:
	case Dir::Right:
		return profile.h;
	case Dir::Up:
	case Dir::Down:
		return profile.v;
	}
	assert(false);
}


uint64_t Loader::_keyForTag(const Tag & tag) noexcept
{
	uint64_t key = 0;
	for (const Dir dir : kAllDirs) {
		assert(!tag[int(dir)].isNull());
		reinterpret_cast<char16_t*>(&key)[int(dir)] = tag[int(dir)].unicode();
	}
	return key;
}


Loader::Loader() :
	normalWallBoxProfile_(kHeavyBoxProfile),
	shortWallBoxProfile_(kLightBoxProfile)
{
}


Loader::CornerForTag Loader::_createCornerForTag() noexcept
{
#if 0
	std::unordered_map<std::string_view, QStringView> map;

	const auto list = [] (const std::function<void(const std::string_view & tag,
			const QStringView & s)> & m) {
		// lrud all normal
		m("nn  ", kCornerNormalLeftRight);
		m("  nn", kCornerNormalUpDown);
		m("n n ", kCornerNormalUpLeft);
		m("n  n", kCornerNormalDownLeft);
		m(" nn ", kCornerNormalUpRight);
		m(" n n", kCornerNormalDownRight);
		m("nnn ", kCornerNormalLeftRightUp);
		m("nn n", kCornerNormalLeftRightDown);
		m("n nn", kCornerNormalUpDownLeft);
		m(" nnn", kCornerNormalUpDownRight);
		m("nnnn", kCornerNormalCross);
	};

	const int count = [&list] () -> int {
		int count = 0;
		list([&count] (const std::string_view &, const QStringView &) {
			count++;
		});
		return count;
	}();

	std::vector<char> tags;
	tags.resize(count * 4);

	{
		int index = 0;
		list([&index, &tags, &map] (const std::string_view & tag, const QStringView & value) {
			char * const key = tags.data() + index * 4;
			std::memcpy(key, tag.data(), 4 * sizeof(char));
			map[std::string_view(key, 4)] = value;
		});
	}

	return CornerForTag {
		.tags = std::move(tags),
		.map = std::move(map),
	};
#else
	static constexpr int kLineCount = std::extent_v<decltype(kBoxSymbols)>;
	static_assert((kLineCount % 3) == 0);

	static constexpr int kRowCount = kLineCount / 3;

	struct Row {
		int count;
		QString lines[3];
	};

	const auto rows = [] () -> std::array<Row, kRowCount> {
		std::array<Row, kRowCount> rows;
		for (int ri = 0; ri < kRowCount; ++ri) {
			Row & row = rows[ri];
			for (int i = 0; i < 3; ++i) {
				const std::string_view & line = kBoxSymbols[ri * 3 + i];
				const QString qline = QString::fromUtf8(line.data(), line.size());
				assert((qline.length() % 3) == 0);
				if (i == 0) {
					row.count = qline.length() / 3;
				} else {
					assert(qline.length() / 3 == row.count);
				}
				row.lines[i] = qline;
			}
		}
		return rows;
	}();

	std::unordered_map<uint64_t, QChar> corners;

	int index = 0;
	for (int ri = 0; ri < kRowCount; ++ri) {
		const Row & row = rows[ri];
		for (int i = 0; i < row.count; ++i) {
			const uint64_t key = _keyForTag({
				row.lines[1][i * 3 + 0],
				row.lines[1][i * 3 + 2],
				row.lines[0][i * 3 + 1],
				row.lines[2][i * 3 + 1],
			});
			const QChar value = row.lines[1][i * 3 + 1];
			assert(!value.isNull());
			assert(value != ' ');
			const auto p = corners.insert({key, value});
			assert(p.second);
			index++;
		}
	}

	for (const BoxProfile & profile : {kLightBoxProfile, kHeavyBoxProfile, kDoubleBoxProfile}) {
		for (int i = 0; i < 16; ++i) {
			Tag tag = {' ', ' ', ' ', ' '};
			int bits = 0;
			for (const Dir dir : kAllDirs) {
				if (i & (1 << int(dir))) {
					tag[int(dir)] = _profileCharForDir(profile, dir);
					bits++;
				}
			}
			const uint64_t key = _keyForTag(tag);
			const auto it = corners.find(key);
			if (i == 0) {
				assert(it == corners.end());
			} else {
				if (bits == 1) {
					if (profile.hasSingle) {
						assert(it != corners.end());
					}
				} else {
					assert(it != corners.end());
				}
			}
		}
	}

	return CornerForTag {
		.corners = std::move(corners),
	};
#endif
}


Map Loader::_createMap(Map::Info && info, std::vector<QString> && lines) noexcept
{
	const int maxLength = [&lines] () -> int {
		int l = 0;
		for (const QString & line : lines) {
			l = std::max(l, line.length());
		}
		assert(l > 3);
		return l;
	}();
	assert(lines.size() > 2);

	assert((maxLength - 1) % 3 == 0);
	const int width = (maxLength - 1) / 3;
	assert(width <= kMaxMapSize);

	assert((lines.size() - 1) % 2 == 0);
	const int height = (lines.size() - 1) / 2;
	assert(height <= kMaxMapSize);

	assert(!info.shortName.empty());

	for (QString & line : lines) {
		while (line.length() < maxLength) {
			line += ' ';
		}
	}

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

	static const std::array<QString, 4> kTileBlocks = {
		QString::fromUtf8("██"),
		QString::fromUtf8("▒▒"),
		kBlockTile,
		kEmptyTile,
	};
	static const std::array<QString, 4> kHorzWallBlocks = {
		QString::fromUtf8("██"),
		QString::fromUtf8("▒▒"),
		kBlockHorzWall,
		QString::fromUtf8("  "),
	};
	static const std::array<QString, 4> kVertWallBlocks = {
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
			if (listContains(kTileBlocks, tile)) {
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
			} else if (listContains(kTraps, tile)) {
				traps.push_back(Trap{Pos{x, y}});
			} else if (tile == kCat) {
				dudes.push_back(Dude {
					.type = Dude::Type::Cat,
					.pos = Pos{x, y},
				});
			} else if (tile == kCatPortal) {
				dudes.push_back(Dude {
					.type = Dude::Type::Cat,
					.pos = Pos{x, y},
				});
				assert(portal.pos == Pos::null());
				portal.pos = Pos{x, y};
			} else if (tile.at(0) == kCat.at(0)) {
				dudes.push_back(Dude {
					.type = Dude::Type::Cat,
					.pos = Pos{x, y},
				});
				teleports.push_back(Teleport {
					.pos = Pos{x, y},
					.color = colorFromString(tile.at(1)),
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
			} else if (tile.at(0) == kSwatGum) {
				gums.push_back(Gum {
					.pos = Pos{x, y},
				});
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
			} else if (tile.at(0) == kPhone) {
				phones.push_back(Phone {
					.pos = Pos{x, y},
					.color = colorFromString(tile.at(1)),
				});
			} else if (tile.at(0) == kTeleport) {
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
			if (listContains(kHorzWallBlocks, wall)) {
				// no wall
			} else if (listContains(kHorzWalls, wall)) {
				hwalls.push_back(Wall {
					.type = Wall::Type::Normal,
					.pos = Pos{x, y},
				});
			} else if (listContains(kHorzShortWalls, wall)) {
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
			} else if (listContains(kHorzZapWalls, wall)) {
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
			if (listContains(kVertWallBlocks, wall)) {
				// no wall
			} else if (listContains(kVertWalls, wall)) {
				vwalls.push_back(Wall {
					.type = Wall::Type::Normal,
					.pos = Pos{x, y},
				});
			} else if (listContains(kVertShortWalls, wall)) {
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
			} else if (listContains(kVertZapWalls, wall)) {
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
		.info = std::move(info),
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


Map Loader::load(const std::filesystem::path & path) const noexcept
{
	static constexpr std::string_view kShortNamePrefix = "name: ";
	static constexpr std::string_view kFullNamePrefix = "full: ";
	static constexpr std::string_view kMoobaaNamePrefix = "moba: ";
	static constexpr std::string_view kTurnsPrefix = "turns: ";

	printf("Loading: %s\n", path.filename().c_str());
	fflush(stdout);

	Map::Info info;
	std::vector<QString> lines;

	{
		std::ifstream file(path);
		assert(file.good());
		std::string line;
		while (std::getline(file, line)) {
			if (line.empty() || std::string_view(line).substr(0, 1) == "#") {
				continue;
			}
			if (line.starts_with(kShortNamePrefix)) {
				assert(info.shortName.empty());
				info.shortName = line.substr(kShortNamePrefix.size());
				assert(info.shortName == makeLower(info.shortName));
				printf("short name: %s\n", info.shortName.c_str());
				fflush(stdout);
				continue;
			}
			if (line.starts_with(kFullNamePrefix)) {
				assert(info.fullName.empty());
				info.fullName = line.substr(kFullNamePrefix.size());
				assert(info.fullName == makeLower(info.fullName));
				printf("full name: %s\n", info.fullName.c_str());
				fflush(stdout);
				continue;
			}
			if (line.starts_with(kMoobaaNamePrefix)) {
				assert(info.moobaaName.empty());
				info.moobaaName = line.substr(kMoobaaNamePrefix.size());
				continue;
			}
			if (line.starts_with(kTurnsPrefix)) {
				assert(info.turns == -1);
				const std::from_chars_result r = std::from_chars(
						line.data() + kTurnsPrefix.size(), line.data() + line.size(), info.turns);
				assert(!std::make_error_condition(r.ec) && info.turns > 0);
				printf("Level turns: %d\n", info.turns);
				continue;
			}
			const QString qline = QString::fromStdString(line);
			lines.push_back(std::move(qline));
		}
	}

	return _createMap(std::move(info), std::move(lines));
}


std::vector<QString> Loader::convert(const Map & map) const noexcept
{
	std::vector<QString> lines;
	lines.resize(map.height * 2 + 1);
	for (QString & line : lines) {
		line.resize(map.width * 3 + 1, QChar(' '));
	}

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

	// tiles
	setTile(map.state.killer.pos, [&map] {
		if (map.portal.pos == map.state.killer.pos) {
			return kKillerPortal;
		}
		if (map.hasGum(map.state.killer.pos)) {
			return kKillerGum;
		}
		return kKiller;
	}());

	if (map.portal.pos != Pos::null()) {
		if (map.portal.pos != map.state.killer.pos) {
			if (map.state.findDude(map.portal.pos) == map.state.dudes.end()) {
				setTile(map.portal.pos, kPortal);
			}
		}
	}

	QString tmpTile;
	tmpTile.resize(2);

	for (const Dude & dude : map.state.dudes) {
		setTile(dude.pos, [&dude, &map, &tmpTile] {
			const bool hasGum = map.hasGum(dude.pos);
			const Teleport * const teleport = [&map, &dude] () -> const Teleport * {
				const auto it = map.findTeleport(dude.pos);
				return it == map.teleports.end() ? nullptr : &*it;
			}();
			const bool hasPortal = dude.pos == map.portal.pos;
			switch (dude.type) {
			case Dude::Type::Victim:
				assert(!hasPortal);
				return hasGum ? kVictimGum : kVictim;
			case Dude::Type::Cat:
				assert(!hasGum);
				if (teleport) {
					tmpTile[0] = kCat[0];
					tmpTile[1] = colorToString(teleport->color);
					return tmpTile;
				} else {
					return hasPortal ? kCatPortal : kCat;
				}
			case Dude::Type::Cop:
				assert(!hasGum);
				assert(!hasPortal);
				tmpTile[0] = kCop[0];
				tmpTile[1] = dirToString(dude.dir);
				return tmpTile;
			case Dude::Type::Swat:
				assert(!hasPortal);
				tmpTile[0] = hasGum ? kSwatGum[0] : kSwat[0];
				tmpTile[1] = dirToString(dude.dir);
				return tmpTile;
			case Dude::Type::Drop:
				assert(!hasGum);
				assert(!hasPortal);
				tmpTile[0] = kDrop[0];
				tmpTile[1] = orientationToString(dude.orientation);
				return tmpTile;
			}
			assert(false);
		}());
	}

	for (const Mine & mine : map.state.mines) {
		setTile(mine.pos, kMine);
	}

	for (const Trap & trap : map.traps) {
		setTile(trap.pos, kTraps[0]);
	}

	for (const Phone & phone : map.phones) {
		tmpTile[0] = kPhone[0];
		tmpTile[1] = colorToString(phone.color);
		setTile(phone.pos, tmpTile);
	}

	for (const Gum & gum : map.gums) {
		if (gum.pos == map.state.killer.pos) continue;
		if (map.state.findDude(gum.pos) != map.state.dudes.end()) continue;
		setTile(gum.pos, kGum);
	}

	for (const Teleport & teleport : map.teleports) {
		if (map.state.findDude(teleport.pos) != map.state.dudes.end()) continue;
		tmpTile[0] = kTeleport[0];
		tmpTile[1] = colorToString(teleport.color);
		setTile(teleport.pos, tmpTile);
	}

	// walls
	for (const Wall & wall : map.hwalls) {
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
			case Wall::Type::Zap: {
				QString zs = QString::fromUtf8("⚡︎");
				// QString zs = QString::fromUtf8("⚡");
				// QString zs = QString::fromUtf8("⌁");
				// QString zs = QString::fromUtf8("z");
				int zsl = zs.length();
				QStringView zz = kVertZapWalls[0];
				int zzl = zz.length();
				return kHorzZapWalls[0];
			}
			}
			assert(false);
		}());
	}

	for (const Wall & wall : map.vwalls) {
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
				return kVertZapWalls[0];
			}
			assert(false);
		}());
	}

	// corners
	const auto cornerForTag = [] () -> std::map<std::string_view, QStringView> {
		std::map<std::string_view, QStringView> m;

		// lrud all normal
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

		// lrud all short
		m["ss  "] = kCornerShortLeftRight;
		m["  ss"] = kCornerShortUpDown;
		m["s s "] = kCornerShortUpLeft;
		m["s  s"] = kCornerShortDownLeft;
		m[" ss "] = kCornerShortUpRight;
		m[" s s"] = kCornerShortDownRight;
		m["sss "] = kCornerShortLeftRightUp;
		m["ss s"] = kCornerShortLeftRightDown;
		m["s ss"] = kCornerShortUpDownLeft;
		m[" sss"] = kCornerShortUpDownRight;
		m["ssss"] = kCornerShortCross;

		// lrud 1 normal and 1 short straight
		m["ns  "] = kCornerShortLeftRight;
		m["  ns"] = kCornerShortUpDown;
		m["sn  "] = kCornerShortLeftRight;
		m["  sn"] = kCornerShortUpDown;

		// lrud 1 normal and 1 short corner
		m["n s "] = kCornerShortUpNormalLeft;
		m["n  s"] = kCornerShortDownNormalLeft;
		m[" ns "] = kCornerShortUpNormalRight;
		m[" n s"] = kCornerShortDownNormalRight;
		m["s n "] = kCornerNormalUpShortLeft;
		m["s  n"] = kCornerNormalDownShortLeft;
		m[" sn "] = kCornerNormalUpShortRight;
		m[" s n"] = kCornerNormalDownShortRight;

		// lrud normal and short straight crosses
		m["nnss"] = kCornerShortUpDownNormalCross;
		m["ssnn"] = kCornerNormalUpDownShortCross;

		// lrud normal and cross corner crosses
		m["snsn"] = kCornerNormalUpDownShortCross;
		m["snns"] = kCornerNormalUpDownShortCross;
		m["nssn"] = kCornerNormalUpDownShortCross;
		m["nsns"] = kCornerNormalUpDownShortCross;

		// lrud normal and short 3-1 crosses
		m["nsss"] = kCornerShortCross;
		m["snss"] = kCornerShortCross;
		m["ssns"] = kCornerShortCross;
		m["sssn"] = kCornerShortCross;
		m["snnn"] = kCornerNormalCross;
		m["nsnn"] = kCornerNormalCross;
		m["nnsn"] = kCornerNormalCross;
		m["nnns"] = kCornerNormalCross;

		// lrud 3 way straight crosses
		m["ssn "] = kCornerShortLeftRightNormalUp;
		m["ss n"] = kCornerShortLeftRightNormalDown;
		m["n ss"] = kCornerShortUpDownNormalLeft;
		m[" nss"] = kCornerShortUpDownNormalRight;
		m["nns "] = kCornerNormalLeftRightShortUp;
		m["nn s"] = kCornerNormalLeftRightShortDown;
		m["s nn"] = kCornerNormalUpDownShortLeft;
		m[" snn"] = kCornerNormalUpDownShortRight;

		// lrud 3 way corner 2 normal 1 short
		m["nsn "] = kCornerNormalLeftRightUp;
		m["n ns"] = kCornerNormalUpDownLeft;
		m["ns n"] = kCornerNormalLeftRightDown;
		m["n sn"] = kCornerNormalUpDownLeft;
		m["snn "] = kCornerNormalLeftRightUp;
		m[" nns"] = kCornerNormalUpDownRight;
		m["sn n"] = kCornerNormalLeftRightDown;
		m[" nsn"] = kCornerNormalUpDownRight;

		// lrud 3 way corner 2 short
		m["sns "] = kCornerShortLeftRightUp;
		m["s sn"] = kCornerShortUpDownLeft;
		m["sn s"] = kCornerShortLeftRightDown;
		m["s ns"] = kCornerShortUpDownLeft;
		m["nss "] = kCornerShortLeftRightUp;
		m[" ssn"] = kCornerShortUpDownRight;
		m["ns s"] = kCornerShortLeftRightDown;
		m[" sns"] = kCornerShortUpDownRight;

		// lrud straight zap
		m["zz  "] = kCornerZap;
		m["  zz"] = kCornerZap;

		// lrud 3 way corner 2 normal 1 zap
		m["nzn "] = kCornerNormalUpLeft;
		m["n nz"] = kCornerNormalUpLeft;
		m["nz n"] = kCornerNormalDownLeft;
		m["n zn"] = kCornerNormalDownLeft;
		m["znn "] = kCornerNormalUpRight;
		m[" nnz"] = kCornerNormalUpRight;
		m["zn n"] = kCornerNormalDownRight;
		m[" nzn"] = kCornerNormalDownRight;

		// lrud 3 way corner 2 short 1 zap
		m["szs "] = kCornerShortUpLeft;
		m["s sz"] = kCornerShortUpLeft;
		m["sz s"] = kCornerShortDownLeft;
		m["s zs"] = kCornerShortDownLeft;
		m["zss "] = kCornerShortUpRight;
		m[" nsz"] = kCornerShortUpRight;
		m["zs s"] = kCornerShortDownRight;
		m[" szs"] = kCornerShortDownRight;

		// lrud 3 way corner 2 short 1 zap
		m["ssz "] = kCornerShortLeftRightUp;
		m["ss z"] = kCornerShortLeftRightDown;
		m["z ss"] = kCornerShortUpDownLeft;
		m[" zss"] = kCornerShortUpDownRight;

		// lrud 1 short 1 zap corner
		m["s z "] = kCornerShortUpLeft;
		m["s  z"] = kCornerShortDownLeft;
		m[" sz "] = kCornerShortUpRight;
		m[" s z"] = kCornerShortDownRight;
		m["z s "] = kCornerShortUpLeft;
		m[" zs "] = kCornerShortUpRight;
		m["z  s"] = kCornerShortDownLeft;
		m[" z s"] = kCornerShortDownRight;

		// lrud 2 straight normal 1 zap
		m["nnz "] = kCornerNormalLeftRight;
		m["nn z"] = kCornerNormalLeftRight;
		m["z nn"] = kCornerNormalUpDown;
		m[" znn"] = kCornerNormalUpDown;

		// lrud 2 normal 2 zap
		m["nnzz"] = kCornerNormalLeftRight;
		m["zznn"] = kCornerNormalUpDown;
		m["nznz"] = kCornerNormalUpLeft;
		m["nzzn"] = kCornerNormalDownLeft;
		m["znnz"] = kCornerNormalUpRight;
		m["znzn"] = kCornerNormalDownRight;

		return m;
	}();

	// const auto makeCorner = [&map, &cornerForTag] (const Pos & pos) -> QStringView {
	// 	const Wall * const rwall = map.findWall(pos, Dir::Up);
	// 	const Wall * const dwall = map.findWall(pos, Dir::Left);
	// 	const Wall * const lwall = map.findWall(pos + Pos{-1, -1}, Dir::Down);
	// 	const Wall * const uwall = map.findWall(pos + Pos{-1, -1}, Dir::Right);

	// 	char cornerTag[4] = {' ', ' ', ' ', ' '};

	// 	const auto tag = [&cornerTag] (const Wall * const wall, const Dir dir) {
	// 		if (wall) {
	// 			char & c = cornerTag[int(dir)];
	// 			if (wall->type == Wall::Type::Normal) {
	// 				c = 'n';
	// 			} else if (wall->type == Wall::Type::Short) {
	// 				c = 's';
	// 			} else if (wall->type == Wall::Type::Zap) {
	// 				c = 'z';
	// 			}
	// 		}
	// 	};

	// 	tag(lwall, Dir::Left);
	// 	tag(rwall, Dir::Right);
	// 	tag(uwall, Dir::Up);
	// 	tag(dwall, Dir::Down);

	// 	const auto it = cornerForTag.find(std::string_view(cornerTag,
	// 			std::extent_v<decltype(cornerTag)>));
	// 	if (it != cornerForTag.end()) {
	// 		return it->second;
	// 	}
	// 	return {};
	// };

	// const auto makeCorner = [&map, this] (const Pos & pos) -> QStringView {
	// 	const Wall * const rwall = map.findWall(pos, Dir::Up);
	// 	const Wall * const dwall = map.findWall(pos, Dir::Left);
	// 	const Wall * const lwall = map.findWall(pos + Pos{-1, -1}, Dir::Down);
	// 	const Wall * const uwall = map.findWall(pos + Pos{-1, -1}, Dir::Right);

	// 	char cornerTag[4] = {' ', ' ', ' ', ' '};

	// 	const auto tag = [&cornerTag] (const Wall * const wall, const Dir dir) {
	// 		if (wall) {
	// 			char & c = cornerTag[int(dir)];
	// 			if (wall->type == Wall::Type::Normal) {
	// 				c = 'n';
	// 			} else if (wall->type == Wall::Type::Short) {
	// 				c = 's';
	// 			} else if (wall->type == Wall::Type::Zap) {
	// 				c = 'z';
	// 			}
	// 		}
	// 	};

	// 	tag(lwall, Dir::Left);
	// 	tag(rwall, Dir::Right);
	// 	tag(uwall, Dir::Up);
	// 	tag(dwall, Dir::Down);

	// 	const auto it = cornerForTag_.map.find(std::string_view(cornerTag,
	// 			std::extent_v<decltype(cornerTag)>));
	// 	if (it != cornerForTag_.map.end()) {
	// 		return it->second;
	// 	}
	// 	return {};
	// };

	const auto makeCorner = [&map, this] (const Pos & pos) -> QChar {
		const Wall * const rwall = map.findWall(pos, Dir::Up);
		const Wall * const dwall = map.findWall(pos, Dir::Left);
		const Wall * const lwall = map.findWall(pos + Pos{-1, -1}, Dir::Down);
		const Wall * const uwall = map.findWall(pos + Pos{-1, -1}, Dir::Right);

		Tag tag = {' ', ' ', ' ', ' '};

		const auto t = [&tag, this] (const Wall * const wall, const Dir dir) {
			if (wall) {
				QChar & c = tag[int(dir)];
				if (wall->type == Wall::Type::Normal) {
					c = _profileCharForDir(normalWallBoxProfile_, dir);
				} else if (wall->type == Wall::Type::Short) {
					c = _profileCharForDir(shortWallBoxProfile_, dir);
				} else if (wall->type == Wall::Type::Zap) {
					c = 'z';
				}
			}
		};

		t(lwall, Dir::Left);
		t(rwall, Dir::Right);
		t(uwall, Dir::Up);
		t(dwall, Dir::Down);

		const uint64_t key = _keyForTag(tag);

		const auto it = cornerForTag_.corners.find(key);
		if (it != cornerForTag_.corners.end()) {
			return it->second;
		}
		return {};
	};

	for (int y = 0; y <= map.height; ++y) {
		for (int x = 0; x <= map.width; ++x) {
			QChar * const s = corner(Pos{x, y});
			const QChar c = makeCorner(Pos{x, y});
			if (!c.isNull()) {
				*s = c;
			} else {
				*s = '$';
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

			bool blocked = !hasSomething(startPos);
			int count = 0;

			while (!queue.empty()) {
				const Pos p = queue.front();
				queue.pop();

				assert(!areaTile(p));
				areaTile(p) = true;
				count++;

				for (const Dir & dir : kAllDirs) {
					const Pos dirPos = p + shiftForDir(dir);
					if (map.contains(dirPos)) {
						bool & a = checkedTile(dirPos);
						if (!a) {
							if (!map.hasAnyWall(p, dir)) {
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

			if (blocked) {
				for (int y = 0; y < map.height; ++y) {
					for (int x = 0; x < map.width; ++x) {
						const Pos pos = Pos{x, y};
						if (areaTile(pos)) {
							setTile(pos, kBlockTile);
							const Pos downPos = pos + shiftForDir(Dir::Down);
							const Pos rightPos = pos + shiftForDir(Dir::Right);
							const Pos downRightPos = downPos + shiftForDir(Dir::Right);
							const bool hasRight = map.contains(rightPos) && areaTile(rightPos);
							const bool hasDown = map.contains(downPos) && areaTile(downPos);
							const bool hasDownRight = map.contains(downRightPos) &&
									areaTile(downRightPos) && hasDown && hasRight;
							if (hasRight) {
								setVertWall(rightPos, kBlockVertWall);
							}
							if (hasDown) {
								setHorzWall(downPos, kBlockHorzWall);
							}
							if (hasDownRight) {
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

	return lines;
}


void Loader::draw(const Map & map) const noexcept
{
	const std::vector<QString> lines = convert(map);
	for (const QString & line : lines) {
		const QByteArray utf8 = line.toUtf8();
		printf("%s\n", utf8.data());
	}
}


void Loader::save(const std::filesystem::path & path, const Map & map) const noexcept
{
	const std::vector<QString> lines = convert(map);

	std::ofstream file(path);
	assert(file.good());

	const auto write = [&file] (const std::string_view & s) {
		file.write(s.data(), s.size());
	};

	const auto writeTag = [&write] (const std::string_view & key, const std::function<void()> & value) {
		write(key);
		write(": ");
		value();
		write("\n");
	};

	const auto writeStringTag = [&writeTag, &write] (const std::string_view & key,
			const std::string_view & value) {
		writeTag(key, [&value, &write] {
			write(value);
		});
	};

	writeStringTag("name", map.info.shortName);
	if (!map.info.fullName.empty()) {
		writeStringTag("full", map.info.fullName);
	}
	if (!map.info.moobaaName.empty()) {
		writeStringTag("moba", map.info.moobaaName);
	}
	if (map.info.turns != -1) {
		writeTag("turns", [&file, &map] {
			file << map.info.turns;
		});
	}

	write("\n");

	for (const QString & line : lines) {
		const QByteArray utf8 = line.toUtf8();
		file.write(utf8.data(), utf8.length());
		write("\n");
	}
}
