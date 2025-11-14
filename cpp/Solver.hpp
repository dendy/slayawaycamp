
#pragma once

#include <functional>
#include <queue>

#include "Map.hpp"




class Solver {
public:
	struct Solution {
		Steps steps;
	};

	using SolutionCallback = std::function<void(Solution && solution)>;

	static void solve(const Map & map, const SolutionCallback & cb);

private:
	struct Move {
		Dir dir;
		int previousId;
		State state;
	};

	struct MoveRes {
		int id;
		bool same;
	};

	Solver(const Map & map, const SolutionCallback & cb);

	std::vector<int> _getSteps(const int moveId) const noexcept;
	std::string _stepsToString(const std::vector<int> & steps) const noexcept;

	int _moveDistance(const int moveId) const noexcept;
	MoveRes _addMove(Move && move);

	std::unordered_map<State, int> moveIdForState_;
	std::vector<Move> moves_;
	std::vector<int> winMoveIds_;
	std::queue<int> moveIdsLeft_;
};




inline int Solver::_moveDistance(const int moveId) const noexcept
{
	int distance = -1;
	for (int id = moveId; id != -1;) {
		id = moves_[id].previousId;
		distance++;
	}
	return distance;
}
