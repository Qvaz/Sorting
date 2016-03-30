#pragma once

#include <vector>
#include <algorithm>
#include <random>
#include <iterator>
#include <thread>
#include <future>

namespace parallel
{
	// classic manual quicksort
	template <typename It, typename Cmp>
	void sort(It begin, It end, Cmp cmp, int threads);

	// sorting using std::partition
	template <typename It, typename Cmp>
	void sort_partition(It begin, It end, Cmp cmp, int threads);

	// sorting using std::nth_element
	template <typename It, typename Cmp>
	void sort_nth_element(It begin, It end, Cmp cmp, int threads);

	// routines
	namespace inner
	{
		// guess what
		template<class Iterator, typename Cmp>
		void insertion_sort( Iterator a, Iterator end, Cmp cmp)
		{
		    std::iter_swap(a, std::min_element(a, end, cmp));
		 
		    for (Iterator b = a; ++b < end; a = b)
		        for(Iterator c = b; cmp(*c, *a); --c, --a)
		            std::iter_swap( a, c );
		}
		
		// not a mean, but a number between the other two
		template <typename El>
		inline auto mean(El a, El b, El c)
		{
			return a < b ? (c < a ? a : b < c ? b : c): (b < c ? b : a < c ? c : a);
		}
		
		// manual partiotion for a quicksort
		template <typename It, typename Cmp>
		std::pair<It, It> partition(It begin, It end, Cmp cmp)
		{
			It middle = begin + (end - begin) / 2;
			auto pivot = inner::mean(*begin, *middle, *std::prev(end));
	
			// classic
			--end;
			while (begin <= end) {
				while (cmp(*begin, pivot)) ++begin;
				while (cmp(pivot, *end)) --end;
				if (begin <= end) {
					if(cmp(*end, *begin)) std::swap(*begin, *end);
					++begin; --end;
				}
			};

			return std::make_pair(end + 1, begin);
		}
	
		//basic sorting algorithm
		template <typename It, typename Cmp, typename Callable>
		void sort_base(It begin, It end, Cmp cmp, Callable partition_call, int threads)
		{
			if (begin == end) return;
			if (end - begin < 30) {
				inner::insertion_sort(begin, end, cmp);
				return;
			}

			// middles are iterators pointing to subranges' bounds
			It middle1, middle2;
			std::tie(middle1, middle2) = partition_call(begin, end, cmp);

			// some threading
			// first subrange is (begin,middle1)
			struct ThreadRAII
			{
				std::thread nested_thread;
				~ThreadRAII() {
					if (nested_thread.joinable()) 
						nested_thread.join();
				}
			} raii_thread;

			if (threads > 1) {
				int new_threads = std::ceil((threads - 1) / 2.);
				raii_thread.nested_thread = std::thread(sort_base<It, Cmp, Callable>, begin, middle1, 
														cmp, partition_call, new_threads);
				threads -= new_threads;
			} 
			else sort_base(begin, middle1, cmp, partition_call, 1);

			// second is (middle2, end)
			sort_base(middle2, end, cmp, partition_call, threads);
		}	
	}

// just pass a manual partiotion as a callback
template <typename It, typename Cmp = std::less<>>
void sort(It begin, It end, Cmp cmp = Cmp{}, int threads = std::thread::hardware_concurrency())
{
	inner::sort_base(begin, end, cmp, inner::partition<It, Cmp>, threads);
}

// two calls of std::partition to gather all elements equal to pivot
template <typename It, typename Cmp = std::less<>>
void sort_partition(It begin, It end, Cmp cmp = Cmp{}, int threads = std::thread::hardware_concurrency())
{
	inner::sort_base(begin, end, cmp, [](It begin, It end, Cmp cmp) {
		It middle = begin + (end - begin) / 2;
		auto pivot = inner::mean(*begin, *middle, *std::prev(end));

		It middle1 = std::partition(begin,   end, [&](const auto& a){ return cmp(a, pivot); });
		It middle2 = std::partition(middle1, end, [&](const auto& a){ return !cmp(pivot, a); });
		return std::make_pair(middle1, middle2);
	}, threads);
}

// always call nth_element for the middle of range
template <typename It, typename Cmp = std::less<>>
void sort_nth_element(It begin, It end, Cmp cmp = Cmp{}, int threads = std::thread::hardware_concurrency())
{
	inner::sort_base(begin, end, cmp, [](It begin, It end, Cmp cmp) {
		It pivot = begin + (end - begin) / 2;

		std::nth_element(begin, pivot, end, cmp);
		return std::make_pair(pivot, pivot);
	}, threads);
}

}