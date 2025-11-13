
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

	void exec() noexcept;

private:
	const Moobaa moobaa_;
	const Map map_;
};
