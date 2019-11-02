#include <iostream>
#include <future>
#include <vector>
#include <string>

#include "ThorSerialize/Traits.h"
#include "ThorSerialize/SerUtil.h"
#include "ThorSerialize/JsonThor.h"
#include "ThorsStream/ThorsStream.h"

using namespace std::string_literals;

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

template<typename T>
class Job
{
    ThorsAnvil::Stream::IThorStream     istream;
    public:
        Job(std::string const& url)
            : istream(url)
        {}
        virtual ~Job()
        {}

        void run()
        {
            using ThorsAnvil::Serialize::jsonImport;
            T data;
            istream >> jsonImport(data);

            processesData(data);
        }

        virtual void processesData(T const& data) = 0;
};

class ListJob: public Job<List>
{
    public:
        using Job<List>::Job;
        virtual void processesData(List const& data) override
        {
            using ThorsAnvil::Serialize::jsonExport;
            std::cout << jsonExport(data) << "\n";
        }
};
class UserJob: public Job<User>
{
    public:
        virtual void processesData(User const& /*data*/) override
        {
        }
};

int main()
{
    using ThorsAnvil::Stream::IThorStream;

    const std::string api       = "https://appsheettest1.azurewebsites.net/sample"s;
    const std::string apiList   = api + "/list"s;
    const std::string apiDetail = api + "/detail/"s;

    std::async([job = std::make_unique<ListJob>(apiList)](){job->run();});

}
