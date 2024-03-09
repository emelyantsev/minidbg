#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <vector>
#include <sstream>

std::vector<std::string> split( const std::string& s, char delimiter ) ;

bool is_prefix( const std::string& s, const std::string& of ) ;

bool is_suffix( const std::string& s, const std::string& of ) ;

#endif