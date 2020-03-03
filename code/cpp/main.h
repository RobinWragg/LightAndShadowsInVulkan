#pragma once

#define SHADOWMAP_RESOLUTION 256
#define MAX_SUBSOURCE_COUNT 16

// Disable warnings for conversions from double to float. Doubles aren't used in this project; disabling these warnings just means I don't have to put an "f" after every float literal.
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )

#include <vector>
using namespace std;

vector<uint8_t> loadBinaryFile(const char *filename);
double getTime();




