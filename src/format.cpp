#include <string>

#include "format.h"

using std::string;

// TODO: Complete this helper function
// INPUT: Long int measuring seconds
// OUTPUT: HH:MM:SS
// REMOVE: [[maybe_unused]] once you define the function
string Format::ElapsedTime(long totalseconds) {
   int seconds = totalseconds % 60;
   int minutes = (totalseconds/60) % 60;
   int hours = (totalseconds/3600) % 24;    
   return std::to_string(hours) + ":" + std::to_string(minutes) + ":" + std::to_string(seconds);
}