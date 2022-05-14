#pragma once

#ifndef EN_LOG_HPP
#define EN_LOG_HPP

#include <ColorfulLogging.h>

	#if not defined(NDEBUG)
	
		#define EN_LOG(message)     { cl::Log(message, cl::Level::Info   ); std::cout << '\n';								 }
		#define EN_WARN(message)    { cl::Log(message, cl::Level::Warning); std::cout << '\n';								 }
		#define EN_ERROR(message)   { cl::Log(message, cl::Level::Error  ); std::cout << '\a'; throw std::runtime_error(""); }
		#define EN_SUCCESS(message) { cl::Log(message, cl::Level::Success); std::cout << '\n';								 }
	
	#else

		#define EN_LOG(message)	    { }
		#define EN_WARN(message)    { }
		#define EN_ERROR(message)   { throw std::runtime_error(""); }
		#define EN_SUCCESS(message) { }
	
	#endif 

#endif