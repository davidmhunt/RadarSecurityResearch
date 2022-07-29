#include "JSONHandler.hpp"

using json = nlohmann::json;

json JSONHandler::parse_JSON(std::string & file_name){
    std::ifstream f(file_name);
    json data;

    if(f.is_open()){
        data = json::parse(f);
        std::string first_name = data["firstName"].get<std::string>();
        std::string last_name = data["lastName"].get<std::string>();
        double pi = data["key values"]["pi"].get<double>();
        std::cout << "First Name: " << first_name << "\n";
        std::cout << "Last Name: " << last_name << "\n";
        std::cout << "key values/pi: " << pi << "\n";
    }
    else{
        std::cerr << "JSONHandler::parse_JSON: Unable to open file\n";
    }

    return data;
}

void JSONHandler::print_file(std::string & file_name){
    std::string line;
    std::ifstream f (file_name);
    if (f.is_open())
    {
        while ( std::getline (f,line) )
        {
        std::cout << line << '\n';
        }
        f.close();
    }

    else std::cerr << "JSONHandler::print_file: Unable to open file\n";
}

