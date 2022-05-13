#include <Core/EnPch.hpp>
#include <Core/Eruption.hpp>

// If compiled in Debug mode or not on Windows, show console
#if not defined(NDEBUG) || not defined(_WIN32)
int main()

// If compiled in Release mode on Windows, don't show the console
#else
#include <windows.h>
	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
#endif
{
	Eruption engine;
	
	try
	{
		engine.Run();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}