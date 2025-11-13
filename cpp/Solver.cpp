
#include "Solver.hpp"

#include "Debug.hpp" // IWYU pragma: keep
#include "Player.hpp"




Solver::Solver(const Map & map)
{
	_addMove(Move {
		.previousId = -1,
		.state = map.state,
	}, false);

	while (!moveIdsLeft_.empty()) {
		const int currentMoveId = moveIdsLeft_.front();
		moveIdsLeft_.pop();

		for (const Dir dir : kAllDirs) {
#ifdef ENABLE_DEBUG
			if (currentMoveId == kDebugMoveId && dir == kDebugMoveDir) {
				int breakhere = 1;
			}
#endif

			const Move & currentMove = moves_[currentMoveId];
			const State & currentState = currentMove.state;

			State state = currentState;

			const Player::Result result = Player::play(map, state, dir);

			const MoveRes moveRes = _addMove(Move {
				.dir = dir,
				.previousId = currentMoveId,
				.state = std::move(state),
			}, result == Player::Result::Win);

			if (result == Player::Result::Win && !moveRes.same) {
				winMoveIds_.push_back(moveRes.id);
			}

#ifdef ENABLE_DEBUG
			// static const std::string_view kExpectedSteps = "rd";
			static const std::string_view kExpectedSteps = "urdludluludruldrd";

			printf("move: from: %d to: %s same: %d id: %d\n", currentMoveId, nameForDir(dir).data(), moveRes.same, moveRes.id);
			if (!moveRes.same) {
				const Move & m = moves_[moveRes.id];
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

			const Move & m = moves_[moveRes.id];
			const std::vector<int> steps = _getSteps(moveRes.id);
			const std::string stepsString = _stepsToString(steps);
			if (stepsString == kExpectedSteps) {
				int b = 1;
			}
#endif
		}
	}

	std::sort(winMoveIds_.begin(), winMoveIds_.end(), [this] (const int a, const int b) {
		return _moveDistance(a) < _moveDistance(b);
	});

	static constexpr int kShowStepsVerbosity = 2;
	static constexpr int kShowStepsCount = -1;

	printf("moves: total: %d win: %d\n", int(moves_.size()), int(winMoveIds_.size()));
	if (kShowStepsVerbosity > 0) {
		for (const int winMoveId : winMoveIds_) {
			std::vector<int> seq;
			for (int id = winMoveId; id != -1;) {
				seq.push_back(id);
				id = moves_[id].previousId;
				if (moves_[id].previousId == -1) break;
			}
			std::reverse(seq.begin(), seq.end());
			printf("win move: %d (steps: %d)\n", winMoveId, _moveDistance(winMoveId));
			if (kShowStepsVerbosity > 1) {
				int stepIndex = 0;
				for (const int id : seq) {
					const Dir dir = moves_[id].dir;
					if (kShowStepsCount == -1 || stepIndex < kShowStepsCount) {
						printf("    % 4d %s\n", id, nameForDir(dir).data());
					}
					stepIndex++;
				}
			}
		}
	}
}


std::vector<int> Solver::_getSteps(const int moveId) const noexcept
{
	std::vector<int> steps;
	for (int id = moveId; id != -1;) {
		steps.push_back(id);
		id = moves_[id].previousId;
		if (moves_[id].previousId == -1) break;
	}
	std::reverse(steps.begin(), steps.end());
	return steps;
}


std::string Solver::_stepsToString(const std::vector<int> & steps) const noexcept
{
	std::string s;
	for (const int id : steps) {
		const Dir dir = moves_[id].dir;
		s += shortNameForDir(dir);
	}
	return s;
}


Solver::MoveRes Solver::_addMove(Move && move, const bool win)
{
	const auto it = moveIdForState_.find(move.state);
	if (it != moveIdForState_.end()) {
		const int oldMoveId = it->second;
		const int oldMoveDistance = _moveDistance(oldMoveId);
		const int newMoveDistance = _moveDistance(move.previousId) + 1;
		if (newMoveDistance < oldMoveDistance) {
			Move & oldMove = moves_[oldMoveId];
			oldMove.previousId = move.previousId;
			oldMove.dir = move.dir;
		}
		return MoveRes {
			.id = oldMoveId,
			.same = true,
		};
	} else {
		const int id = moves_.size();
		moveIdForState_[move.state] = id;
		moves_.push_back(std::move(move));
		if (!win) {
			moveIdsLeft_.push(id);
		}
		return MoveRes {
			.id = id,
			.same = false,
		};
	}
};
