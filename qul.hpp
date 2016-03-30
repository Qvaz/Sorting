#pragma once

namespace qul
{
	template <typename Fn>
	struct RAII
	{
		Fn fn;

		RAII(Fn _fn) : fn(_fn) {}
		~RAII() { fn(); }
	};

	template <typename Fn>
	RAII<Fn> make_raii(Fn fn) {
		return RAII<Fn>(fn);
	}
}