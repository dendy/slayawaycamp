
#pragma once

#include <filesystem>
#include <cassert>

#include "Map.hpp"
#include "Moobaa.hpp"




class App {
public:
	struct Args {
		std::filesystem::path mapFilePath;
		bool moobaa = false;
	};

	App(Args && args);

private:
	void _execMap() noexcept;
	void _execMoobaa() noexcept;

	const Moobaa moobaa_;
	const Map map_;
};
