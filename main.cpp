#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include <iterator>
#include <chrono>

#include "parallel.hpp"

// just execute the algorithm, find out the duration and validate the result
template <typename Cmp, typename Sort, typename Vector, typename... Args>
auto benchmark(Sort sort, Vector vec, Args&&... args)
{
	auto start = std::chrono::system_clock::now();
	sort(vec.begin(), vec.end(), Cmp{}, std::forward<Args>(args)...);
	auto end = std::chrono::system_clock::now();
	auto duration = end - start;

	bool validation_result = std::is_sorted(vec.begin(), vec.end(), Cmp{});

	auto duration_result = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

	return std::make_pair(validation_result, duration_result);
}

template <typename Cmp = std::less<>, typename Sort, typename... Args>
void test(Sort sort, Args&&... args)
{
	// test sample length and number of iterations
	const size_t length = 1e8;
	const int cycles = 10;

	std::random_device rd;
	std::mt19937 g(rd());
	std::uniform_int_distribution<> dist;

	// benchmarks algorithm 'cycles' times and accumulate the duration
	double sum = 0;
	bool sorted = true;
	for (int i = 0; i < cycles; ++i) {
		std::vector<int> vec(length);
		std::generate(vec.begin(), vec.end(), [&](){ return dist(g); });

		auto res = benchmark<Cmp>(sort, std::move(vec), std::forward<Args>(args)...);

		if (!res.first) {
			sorted = false;
			break;
		}

		sum += res.second;		
	}

	// and find mean time
	double mean = sum / cycles;

	std::cout << "Sorted: " << std::boolalpha << sorted << std::endl;
	std::cout << "Time: " << mean << " ms \n";
}

// macro magic for template function passing as a callback
#define SORT_CALL(_SORT_NAME_) [](auto begin, auto end, auto cmp, auto&&... args) \
									{ (_SORT_NAME_)(begin, end, cmp, args...);}

int main(int argc, char* argv[])
{
	const unsigned threads = 8;

	std::cout << "parallel " << threads << ":\n";
	test<std::greater<>>(SORT_CALL(parallel::sort), threads);

	std::cout << "\nparallel partition " << threads << ":\n";
	test<std::greater<>>(SORT_CALL(parallel::sort_partition), threads);
	
	std::cout << "\nparallel nth_element " << threads << ":\n";
	test(SORT_CALL(parallel::sort_nth_element), threads);
	
	std::cout << "\nstd:\n";
	test(SORT_CALL(std::sort));
}
