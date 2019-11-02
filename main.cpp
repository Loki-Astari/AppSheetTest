#include <iostream>
#include "ThorSerialize/Traits.h"
#include "ThorSerialize/JsonThor.h"
#include "ThorsStream/ThorsStream.h"

class List
{
    private:
        std::vector<int>        result;
        std::string             token;
        friend class ThorsAnvil::Serialize::Traits<List>;
    public:
};

class User
{
    private:
        int                     id;
        std::string             name;
        int                     age;
        std::string             number;
        std::string             photo;
        std::string             bio;
        friend class ThorsAnvil::Serialize::Traits<User>;
    public:
};

ThorsAnvil_MakeTrait(List, result, token);
ThorsAnvil_MakeTrait(User, id, name, age, number, photo, bio);


int main()
{
    using ThorsAnvil::Serialize::jsonExport;
    using ThorsAnvil::Serialize::jsonImport;
    using ThorsAnvil::Stream::IThorStream;


    std::cout << "App Sheet Test\n";
    std::stringstream testStream;
    testStream << R"({"id":1,"name":"bill","age":39,"number":"555-555-5555","photo":"https://appsheettest1.azurewebsites.net/male-16.jpg","bio":"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed ex sapien, interdum sit amet tempor sit amet, pretium id neque. Nam ultricies ac felis ut lobortis. Praesent ac purus vitae est dignissim sollicitudin. Duis iaculis tristique euismod. Nulla tellus libero, gravida sit amet nisi vitae, ultrices venenatis turpis. Morbi ut dui nunc."})";

    std::cout << testStream.str() << "\n";

    User    user1;
    testStream >> jsonImport(user1);
    std::cout << jsonExport(user1) << "\n";


    IThorStream stream("https://appsheettest1.azurewebsites.net/sample/detail/12");
    stream >> jsonImport(user1);
    std::cout << jsonExport(user1) << "\n";
}
