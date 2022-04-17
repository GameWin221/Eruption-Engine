#include <Core/EnPch.hpp>
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
		std::cerr << e.what() << '\n';

		return EXIT_FAILURE;
	}
}