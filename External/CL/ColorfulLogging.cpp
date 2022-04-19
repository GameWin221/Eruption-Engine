// Made by Mateusz Antkiewicz
// Github Repository: https://github.com/GameWin221/Colorful-Logging

#include "ColorfulLogging.h"

namespace cl
{

#ifdef _WIN32
	// The default console window
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	// A map of running benchmarks
	std::map<std::string, std::chrono::system_clock::time_point> benchmarks;


	// Returns current system time
	std::string Time()
	{
		time_t     now = time(0);
		struct tm  tstruct;
		char       buffer[10];
		tstruct = *localtime(&now);
		strftime(buffer, sizeof(buffer), "%X", &tstruct);

		return buffer;
	}

	// Sets the color of printed console text
	void SetConsoleColor(Color color)
	{
#ifdef _WIN32
		SetConsoleTextAttribute(hConsole, static_cast<int>(color));
#else
		std::cout << "\033[" << static_cast<int>(color) << "m";
#endif
	}

	// Prints text (and time if 'logTime' is true) on the default console. 'importance' is the color in which the text will be printed.
	void Log(std::string text, Level importance, bool logTime)
	{
		// Casting from Level to Color enum
		SetConsoleColor(static_cast<Color>(importance));

		if (logTime)
			std::cout << "[" << Time() << "] " << text;
		else
			std::cout << text;

		SetConsoleColor(Color::DarkWhite);
	}




	// BENCHMARKING

		// Creates a 'bechmarkName' benchmark which contains the time of its start
	void BenchmarkBegin(std::string bechmarkName)
	{
		if (benchmarks.count(bechmarkName) == 0)
		{
			benchmarks[bechmarkName] = std::chrono::system_clock::now();
		}

#ifdef DEBUG_MODE
		else
			Log("[CL] Failed to begin a benchmark - Benchmark called \"" + bechmarkName + "\" already exists!\n", Level::Warning);
#endif
	}

	// Returns time elapsed since the benchmark's start in seconds. Will return -1.0 if 'benchmarkName' is invalid.
	double BenchmarkGetTime(std::string bechmarkName)
	{
		if (benchmarks.count(bechmarkName) != 0)
		{
			auto now = std::chrono::system_clock::now();
			std::chrono::duration<double> diff = now - benchmarks[bechmarkName];
			return diff.count();
		}
		else
		{
#ifdef DEBUG_MODE
			Log("[CL] Failed to get time of a benchmark - Benchmark called \"" + bechmarkName + "\" doesn't exists!\n", Level::Warning);
#endif
			return -1.0;
		}
	}

	// Resets the 'benchmarkName' benchmark. Returns time elapsed since the benchmark's start in seconds. Will return -1.0 if 'benchmarkName' is invalid.
	double BenchmarkReset(std::string bechmarkName)
	{
		if (benchmarks.count(bechmarkName) != 0)
		{
			double time = BenchmarkGetTime(bechmarkName);
			benchmarks[bechmarkName] = std::chrono::system_clock::now();
			return time;
		}
		else
		{
#ifdef DEBUG_MODE
			Log("[CL] Failed to reset a benchmark - Benchmark called \"" + bechmarkName + "\" doesn't exists!\n", Level::Warning);
#endif
			return -1.0;
		}
	}

	// Removes the 'benchmarkName' benchmark. Returns time elapsed since the benchmark's start in seconds. Will return -1.0 if 'benchmarkName' is invalid.
	double BenchmarkStop(std::string bechmarkName)
	{
		if (benchmarks.count(bechmarkName) != 0)
		{
			double time = BenchmarkGetTime(bechmarkName);
			benchmarks.erase(bechmarkName);
			return time;
		}
		else
		{
#ifdef DEBUG_MODE
			Log("[CL] Failed to stop a benchmark - Benchmark called \"" + bechmarkName + "\" doesn't exists!\n", Level::Warning);
#endif
			return -1.0;
		}
	}

	// Stops all currently running benchmarks.
	void BenchmarkStopAll()
	{
		benchmarks.clear();
	}
}