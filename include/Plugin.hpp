#pragma once

#include <liblib/liblib.hpp>

namespace zany {
	
	class Plugin : public liblib::Library {
	public:
		Plugin(std::string name);
		~Plugin();
	};
	
}
