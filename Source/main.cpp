#include <Core/EnPch.hpp>
#include <Core/Eruption.hpp>

// If compiled in Debug mode or not on Windows, show console
#if defined(_DEBUG) || not defined(_WIN32)
int main()

// If compiled in Release mode on Windows, don't show the console
#else
	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#endif
{
	Eruption engine;
	
	try 
	{ 
		engine.Run(); 
	}
	catch (const std::exception& e) 
	{
		return EXIT_FAILURE;
	}
}