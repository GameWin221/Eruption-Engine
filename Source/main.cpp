#include <Core/Eruption.hpp>

int main()
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