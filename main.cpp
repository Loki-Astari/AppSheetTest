#include <iostream>
#include <future>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <regex>
#include <mutex>

#include "ThorSerialize/Traits.h"
#include "ThorSerialize/SerUtil.h"
#include "ThorSerialize/JsonThor.h"
#include "ThorsStream/ThorsStream.h"

using namespace std::string_literals;

// Some global constants.
const std::string api       = "https://appsheettest1.azurewebsites.net/sample"s;
const std::string apiList   = api + "/list"s;
const std::string apiDetail = api + "/detail/"s;
const std::regex  phoneNumber("^[0-9][0-9][0-9][- ][0-9][0-9][0-9][- ][0-9][0-9][0-9][0-9]$");

// In this app List and User
// are simply property bags no nead to have access functions.
// If this was a more complex app then we would consider having other methods.
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

// Set up comparison functions used on user.
const auto youngestUser = [](User const& lhs, User const& rhs){return lhs.age < rhs.age;};
const auto nameTest     = [](User const& lhs, User const& rhs){return lhs.name < rhs.name;};

// Set up List and User to be read from JSON stream.
// See: jsonImport() and jsonExport() below
ThorsAnvil_MakeTrait(List, result, token);
ThorsAnvil_MakeTrait(User, id, name, age, number, photo, bio);


// A generic Job.
// Simply reads an object from an istream.
// If the read worked then processes it.
// Note: An istream treats a CURL socket like a standard C++ stream.
template<typename T>
class Job
{
    protected:
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

// A job to handle the details from getting a user object.
class UserJob: public Job<User>
{
    public:
        using Job<User>::Job;
        virtual void processesData(std::vector<User>& users, User const& user) override
        {
            // Check if the phone number is OK.
            if (std::regex_search(user.number, phoneNumber)) {

                // Mutex shared across all objects (notice the static).
                static std::mutex  mutex;

                // Lock the mutex when modifying "users"
                std::lock_guard<std::mutex>   lock(mutex);

                // Add the user to a heap.
                // The heap is ordered by youngest person.
                users.emplace_back(std::move(user));
                std::push_heap(users.begin(), users.end(), youngestUser);
                if (users.size() == 6) {
                    // If we have more than 5 people the pop the oldest one off.
                    // Thus we maintain a heap of the 5 youngest people.
                    std::pop_heap(users.begin(), users.end(), youngestUser);
                    users.pop_back();
                }
            }
        }
};

// A job to handle the list object.
class ListJob: public Job<List>
{
    std::vector<std::future<void>>            userFutures;
    public:
        using Job<List>::Job;
        virtual void processesData(std::vector<User>& users, List const& data) override
        {
            for(auto const& userId: data.result) {
                // For each user add a job ("UserJob") to the async queue.
                userFutures.emplace_back(std::async([&users, job = std::make_unique<UserJob>(apiDetail + std::to_string(userId))](){job->run(users);}));
            }
            if (data.token.get()) {
                istream = ThorsAnvil::Stream::IThorStream(apiList + "?token=" + *data.token);
                run(users);
            }
            else {
                for(auto& future: userFutures) {
                    future.wait();
                }
            }
        }
};

int main()
{
    std::vector<User>   users;

    ListJob listJob(apiList);
    listJob.run(users);

    std::sort(users.begin(), users.end(), nameTest);
    using ThorsAnvil::Serialize::jsonExport;
    std::cout << jsonExport(users) << "\n";
}
