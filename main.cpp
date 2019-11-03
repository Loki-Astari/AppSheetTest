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
const int         maxParrallelism = 20;

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
// Note: youngestUser uses both name and age. This is because if we have a lot of people at the same age we want to keep the
//       lexicographically lowest names as we eventually will sort by name.
const auto youngestUser = [](User const& lhs, User const& rhs){return std::forward_as_tuple(lhs.age, lhs.name) < std::forward_as_tuple(rhs.age, rhs.name);};
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

        void run()
        {
            bool hasMore = false;
            do
            {
                T data;
                using ThorsAnvil::Serialize::jsonImport;
                if (istream >> jsonImport(data)) {
                    processesData(data);
                    hasMore = moreData(data);
                }
                else {
                    // Do some error handling
                }
            }
            while(hasMore);
        }

        virtual void processesData(T const& data) = 0;
        virtual bool moreData(T const&) {return false;}
};

class JobHolder;

// A job to handle the details from getting a user object.
class UserJob: public Job<User>
{
    JobHolder&      jobHolder;
    public:
        UserJob(std::string const& url, JobHolder& jobHolder)
            : Job(url)
            , jobHolder(jobHolder)
        {}
        virtual void processesData(User const& user) override;
};

class JobHolder
{
    std::vector<User>&                          users;
    std::map<int, std::future<void>>            userFutures;
    std::mutex                                  mutex;
    std::condition_variable                     cond;
    int                                         lastFinished;
    bool                                        justWaiting;
    public:
        JobHolder(std::vector<User>& users)
            : users(users)
            , lastFinished(-1)
            , justWaiting(false)
        {}
        void addJob(int userId)
        {
            // Lock the mutex when modifying "users"
            std::unique_lock<std::mutex>     lock(mutex);

            // No more jobs if we are waiting.
            if (justWaiting) {
                return; 
            }

            // We don't want to add more then maxParrallelism
            // simply because we don't want userFutures to blow up in memory to infinite size.
            // Note: Behind the scenes the parallelism is controlled for us by the implementation.
            cond.wait(lock, [&userFutures = this->userFutures](){return userFutures.size() < maxParrallelism;});

            // Start async job to create and handle connection.
            userFutures.emplace(userId, std::async([job = std::make_unique<UserJob>(apiDetail + std::to_string(userId), *this)](){job->run();}));
        }
        void addResult(User const& user)
        {
            if (std::regex_search(user.number, phoneNumber)) {

                // Lock the mutex when modifying "users"
                std::unique_lock<std::mutex>   lock(mutex);

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

                // If we are waiting then a thread is in waitForAllJobs
                // So we can't remove items from the userFutures as it is being used.
                if (!justWaiting) {
                    if (lastFinished != -1) {
                        // Note: Can't remove the current one (user.id)
                        //       As we are still in the thread that the future belongs too.
                        //       So we remove the last lastFinished and note this lastFinished
                        //       so it will be removed next time.
                        userFutures.erase(lastFinished);
                        cond.notify_one();
                    }
                    lastFinished = user.id;
                }
            }
        }
        void waitForAllJobs()
        {
            {
                std::unique_lock<std::mutex>     lock(mutex);
                justWaiting = true;
            }

            for(auto& future: userFutures) {
                future.second.wait();
            }
        }
};

// A job to handle the list object.
class ListJob: public Job<List>
{
    JobHolder   jobHolder;
    public:
        ListJob(std::string const& url, std::vector<User>& result)
            : Job(url)
            , jobHolder(result)
        {}
        virtual void processesData(List const& data) override;
        virtual bool moreData(List const& data) override;
};

void UserJob::processesData(User const& user)
{
    jobHolder.addResult(user);
}

void ListJob::processesData(List const& data)
{
    for(auto const& userId: data.result) {
        // For each user add a job ("UserJob") to the async queue.
        jobHolder.addJob(userId);
    }
}

bool ListJob::moreData(List const& data)
{
    if (data.token.get()) {
        istream = ThorsAnvil::Stream::IThorStream(apiList + "?token=" + *data.token);
        return true;
    }
    else {
        jobHolder.waitForAllJobs();
        return false;
    }
}

int main()
{
    std::vector<User>   users;

    ListJob listJob(apiList, users);
    listJob.run();

    std::sort(users.begin(), users.end(), nameTest);
    using ThorsAnvil::Serialize::jsonExport;
    std::cout << jsonExport(users) << "\n";
}
