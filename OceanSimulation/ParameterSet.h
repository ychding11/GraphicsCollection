#ifndef PARAMETER_SET_H_
#define PARAMETER_SET_H_

#include <map>
#include <string>

class ParameterSet
{
private:
    std::map<std::string, int> iparameters;
    std::map<std::string, float> fparameters;
    std::map<std::string, std::string> fparameters;

public:
    ParameterSet() { }



};

#endif