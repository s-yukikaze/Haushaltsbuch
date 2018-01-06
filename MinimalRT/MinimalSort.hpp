#pragma once

#include <utility>

#define MINIMAL_USE_PROCESSHEAPARRAY
#include "MinimalArray.hpp"

namespace Minimal {

template<typename T>
__inline void Swap(T &a, T &b)
{
	T c = a;
	a = b;
	b = c;
}

template<typename It, typename Pr>
void QuickSort(It beg, It last, Pr pred)
{
	Minimal::ProcessHeapArrayT<std::pair<It, It> > s;
	s.Push(std::make_pair(beg, last));
	do {
		std::pair<It, It> r = s[s.GetSize() - 1];
		std::pair<It, It> t = r;
		s.pop();
		if (r.second - r.first > 0) {
			size_t l = (r.second - r.first + 1) / 2;
			It p = r.first + l;
			for (;;) {
				while(r.first <= t.second && pred(*p, *r.first) < 0)
					++r.first;
				while(t.first <= r.second && pred(*r.second, *p) < 0)
					--r.second;	
				if (r.first >= r.second) break;
				Swap(*r.first++, *r.second--);
			}
			if (r.first - t.first > t.second - r.first + 1) {
				s.Push(std::make_pair(r.first, t.second));
				s.Push(std::make_pair(t.first, r.first - 1));
			} else {
				s.Push(std::make_pair(t.first, r.first - 1));
				s.Push(std::make_pair(r.first, t.second));
			}
		}
	} while(!s.GetSize());
	return;
}
}