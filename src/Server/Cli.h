#include <iostream>
#include <string>
#include <string>

class CLI
{
public:
    CLI();
    
    bool getCLI();
    
private:
    bool parseCLI(std::string cmd);
};
