
#include "Solver.hpp"

#include "Debug.hpp" // IWYU pragma: keep
#include "Player.hpp"




void Solver::solve(const Map & map, const SolutionCallback & cb)
{
	Solver(map, cb);
}


Solver::Solver(const Map & map, const SolutionCallback & cb)
{
	{
		const MoveRes init = _addMove(Move {
			.previousId = -1,
			.state = map.state,
		});
		assert(!init.same);
		moveIdsLeft_.push(init.id);
	}

	while (!moveIdsLeft_.empty()) {
		const int currentMoveId = moveIdsLeft_.front();
		moveIdsLeft_.pop();

		const int currentDistance = _moveDistance(currentMoveId);
		const bool isLastTurn = map.info.turns != -1 && currentDistance == map.info.turns - 1;

		for (const Dir dir : kAllDirs) {
#ifdef ENABLE_DEBUG
			if (currentMoveId == kDebugMoveId && dir == kDebugMoveDir) {
				int breakhere = 1;
			}
#endif

			State state = moves_[currentMoveId].state;

			const Player::Result result = Player::play(map, state, dir);

			if (result == Player::Result::Fail) {
				continue;
			}

			const MoveRes moveRes = _addMove(Move {
				.dir = dir,
				.previousId = currentMoveId,
				.state = std::move(state),
			});

			if (!moveRes.same) {
				if (result == Player::Result::Win) {
					winMoveIds_.push_back(moveRes.id);
				} else {
					if (!isLastTurn) {
						moveIdsLeft_.push(moveRes.id);
					}
				}
			}

#ifdef ENABLE_DEBUG
			const std::vector<int> steps = _getSteps(moveRes.id);
			const std::string stepsString = _stepsToString(steps);

			if (!kDebugOnlyExpectedSteps || kDebugExpectedSteps.starts_with(stepsString)) {
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
			}
			fflush(stdout);

			const Move & m = moves_[moveRes.id];
			if (stepsString == kDebugExpectedSteps) {
				int b = 1;
			}
#endif
		}
	}

	std::sort(winMoveIds_.begin(), winMoveIds_.end(), [this] (const int a, const int b) {
		return _moveDistance(a) < _moveDistance(b);
	});

	printf("moves: %d wins: %d\n", int(moves_.size()), int(winMoveIds_.size()));

	for (const int winMoveId : winMoveIds_) {
		std::vector<int> seq;
		for (int id = winMoveId; id != -1;) {
			seq.push_back(id);
			id = moves_[id].previousId;
			if (moves_[id].previousId == -1) break;
		}
		std::reverse(seq.begin(), seq.end());

		if (kShowStepsVerbosity > 0) {
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

		cb(Solution {
			.steps = [this, &seq] () -> Steps {
				Steps steps;
				for (const int id : seq) {
					steps.push_back(moves_[id].dir);
				}
				return steps;
			}(),
		});
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


Solver::MoveRes Solver::_addMove(Move && move)
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
		return MoveRes {
			.id = id,
			.same = false,
		};
	}
};
