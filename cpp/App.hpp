
#pragma once

#include <filesystem>
#include <cassert>

#include "Loader.hpp"
#include "Map.hpp"
#include "Moobaa.hpp"




class App {
public:
	struct Args {
		std::filesystem::path mapFilePath;
		bool moobaa = false;
		bool convert = false;
	};

	App(Args && args);

private:
	void _execMap() noexcept;
	void _execMoobaa() noexcept;
	void _execConvert() noexcept;

	const Loader loader_;
	const Moobaa moobaa_;
	const Map map_;
};
