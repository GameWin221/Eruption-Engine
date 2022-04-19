// Made by Mateusz Antkiewicz
// Github Repository: https://github.com/GameWin221/Colorful-Logging

#pragma once

#ifndef CL_INCLUDED
#define CL_INCLUDED

#pragma warning(disable : 4996)

// Uncomment the line below to enable the debug mode. Debug mode will enable CL's errors logging.
//#define DEBUG_MODE

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <iostream>
#include <string>
#include <map>
#include <chrono>

// Colorful Logging
namespace cl
{

// UTILITY
#ifdef _WIN32
	enum struct Color
	{
		Blank,
		DarkBlue,
		DarkGreen,
		DarkCyan,
		DarkRed,
		DarkPink,
		DarkYellow,
		DarkWhite,
		Grey,
		Blue,
		Green,
		Cyan,
		Red,
		Pink,
		Yellow,
		White,
		Default = DarkWhite
	};
#else
	enum struct Color
	{
		Blank = 30,
		DarkBlue = 34,
		DarkGreen = 32,
		DarkCyan = 36,
		DarkRed = 31,
		DarkPink = 35,
		DarkYellow = 33,
		DarkWhite = 37,
		Grey = 90,
		Blue = 94,
		Green = 92,
		Cyan = 96,
		Red = 91,
		Pink = 95,
		Yellow = 93,
		White = 97,
		Default = DarkWhite
	};
#endif

	enum struct Level
	{
		Info    = (int)Color::White,
		Warning = (int)Color::Yellow,
		Error   = (int)Color::Red,
		Success = (int)Color::Green
	};


// LOGGING

	// Returns current system time
	extern std::string Time();

	// Sets the color of printed console text
	extern void SetConsoleColor(Color color);

	// Prints text (and time if 'logTime' is true) on the default console. 'importance' is the color in which the text will be printed.
	extern void Log(std::string text, Level importance, bool logTime = true);




// BENCHMARKING

	// Creates a 'bechmarkName' benchmark which contains the time of its start
	extern void BenchmarkBegin(std::string bechmarkName);

	// Returns time elapsed since the benchmark's start in seconds. Will return -1.0 if 'benchmarkName' is invalid.
	extern double BenchmarkGetTime(std::string bechmarkName);

	// Resets the 'benchmarkName' benchmark. Returns time elapsed since the benchmark's start in seconds. Will return -1.0 if 'benchmarkName' is invalid.
	extern double BenchmarkReset(std::string bechmarkName);

	// Removes the 'benchmarkName' benchmark. Returns time elapsed since the benchmark's start in seconds. Will return -1.0 if 'benchmarkName' is invalid.
	extern double BenchmarkStop(std::string bechmarkName);

	// Stops all currently running benchmarks.
	extern void BenchmarkStopAll();
}
#endif