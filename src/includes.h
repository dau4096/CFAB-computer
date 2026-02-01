#ifndef INCLUDES_H
#define INCLUDES_H


//OS Specific
#ifdef _WIN32
//Include Windows.
#include <Windows.h> //Only for windows systems (obviously). Only needed for console-specific functions, which are minimal.
#elif defined(__linux__)
//Include Linux things.
#include <unistd.h>		//Both required for the console output mode.
#include <sys/ioctl.h>  // ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^
#endif


//GLM
#include <glm/glm.hpp>


//Include std subheaders.
//IO
#include <iostream>
#include <fstream>
#include <sstream>

//Types
#include <cstdint>
#include <bitset>
#include <string>
#include <cstring>
#include <cmath>
#include <array>
#include <vector>
#include <functional>

//Time
#include <chrono>
#include <thread>

//Exct
#include <cassert>

#endif