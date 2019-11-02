#include <iostream>
#include <future>
#include <vector>
#include <string>
#include <memory>

#include "ThorSerialize/Traits.h"
#include "ThorSerialize/SerUtil.h"
#include "ThorSerialize/JsonThor.h"
#include "ThorsStream/ThorsStream.h"

using namespace std::string_literals;

const std::string api       = "https://appsheettest1.azurewebsites.net/sample"s;
const std::string apiList   = api + "/list"s;
const std::string apiDetail = api + "/detail/"s;

struct List
{
    std::vector<int>                result;
    std::unique_ptr<std::string>    token;
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
            if (istream >> jsonImport(data)) {
                processesData(data);
            }
            else {
                // Do some error handling
            }
        }

        virtual void processesData(T const& data) = 0;
};

class UserJob: public Job<User>
{
    public:
        using Job<User>::Job;
        virtual void processesData(User const& user) override
        {
            using ThorsAnvil::Serialize::jsonExport;
            std::cout << jsonExport(user);
        }
};

class ListJob: public Job<List>
{
    public:
        using Job<List>::Job;
        virtual void processesData(List const& data) override
        {
            using ThorsAnvil::Serialize::jsonExport;
            std::cout << jsonExport(data);
            if (data.token.get()) {
                std::async([job = std::make_unique<ListJob>(apiList + "?token=" + *data.token)](){job->run();});
            }
            for(auto const& userId: data.result) {
                std::async([job = std::make_unique<UserJob>(apiDetail + std::to_string(userId))](){job->run();});
            }
        }
};

int main()
{
    using ThorsAnvil::Stream::IThorStream;

    std::async([job = std::make_unique<ListJob>(apiList)](){job->run();});

}
