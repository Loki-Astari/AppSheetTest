#include <iostream>
#include <future>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <regex>

#include "ThorSerialize/Traits.h"
#include "ThorSerialize/SerUtil.h"
#include "ThorSerialize/JsonThor.h"
#include "ThorsStream/ThorsStream.h"

using namespace std::string_literals;

const std::string api       = "https://appsheettest1.azurewebsites.net/sample"s;
const std::string apiList   = api + "/list"s;
const std::string apiDetail = api + "/detail/"s;
const std::regex  phoneNumber("^[0-9][0-9][0-9][- ][0-9][0-9][0-9][- ][0-9][0-9][0-9][0-9]$");

struct List
{
    std::vector<int>                result;
    std::unique_ptr<std::string>    token;
};

struct User
{
        int                     id;
        std::string             name;
        int                     age;
        std::string             number;
        std::string             photo;
        std::string             bio;

};

const auto youngestUser = [](User const& lhs, User const& rhs){return lhs.age < rhs.age;};
const auto nameTest     = [](User const& lhs, User const& rhs){return lhs.name < rhs.name;};

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

        void run(std::vector<User>& result)
        {
            using ThorsAnvil::Serialize::jsonImport;
            T data;
            if (istream >> jsonImport(data)) {
                processesData(result, data);
            }
            else {
                // Do some error handling
            }
        }

        virtual void processesData(std::vector<User>& result, T const& data) = 0;
};

class UserJob: public Job<User>
{
    public:
        using Job<User>::Job;
        virtual void processesData(std::vector<User>& users, User const& user) override
        {
            if (std::regex_search(user.number, phoneNumber)) {
                users.emplace_back(std::move(user));
                std::push_heap(users.begin(), users.end(), youngestUser);
                if (users.size() == 6) {
                    std::pop_heap(users.begin(), users.end(), youngestUser);
                    users.pop_back();
                }
            }
        }
};

class ListJob: public Job<List>
{
    public:
        using Job<List>::Job;
        virtual void processesData(std::vector<User>& users, List const& data) override
        {
            if (data.token.get()) {
                std::async([&users, job = std::make_unique<ListJob>(apiList + "?token=" + *data.token)](){job->run(users);});
            }
            for(auto const& userId: data.result) {
                std::async([&users, job = std::make_unique<UserJob>(apiDetail + std::to_string(userId))](){job->run(users);});
            }
        }
};

int main()
{
    using ThorsAnvil::Stream::IThorStream;
    std::vector<User>   users;

    std::async([&users, job = std::make_unique<ListJob>(apiList)](){job->run(users);});
    // This will not return until all async jobs have completed.

    std::sort(users.begin(), users.end(), nameTest);
    using ThorsAnvil::Serialize::jsonExport;
    std::cout << jsonExport(users) << "\n";
}
