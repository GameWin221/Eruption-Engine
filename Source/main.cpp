#include <Core/EnPch.hpp>
#include <Core/Eruption.hpp>

// If compiled in Debug mode, show console
#ifdef _DEBUG
int main()

// If compiled in Release mode, don't show console
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