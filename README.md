
## App Sheet Test App

### Build instructions.

./configure
make

### Requirements

You will need to have curl installed.

    > #On a mac
    > brew intall curl

    > #On a Linux
    > sudo app-get install curl   # Or your current package management system


You will need to install a C++ compiler than can cope with C++ 17.


### How It Works.
This application uses two external packages (Open Source). It uses the header only versions of these libraries so no testing is done to validate these packages work on the current system. Both these open source packages have been tested on Apple/Mac using the latest XCode compiler. Both packages use travis.ci to build on mac and Linux (though an older version). See travis.ci for details.


#### ThorsSerializer.
This is used to convert C++ objects to/from JSON without any intermediate code.

#### ThorsStream
Wraps CURL handles to give them a C++ stream interface.  
By creating an IThorStream object it connects to the URL and reads the data from the URL making the data appear like a standard stream object.  
Any errors on the stream will make set the std::ios::badflag on the stream state thus causing any stream operation to fail. This is used to prevent reading using objects that failed to read correctly.

Note: behind the scenes IThorStream is using the curl multi handle to allow multiple connections to be handled simultaneously by a single thread (that is not the master thread). Any attempt to read from a stream that has an empty buffer will cause that thread to release control until the buffer has been filled.

#### Async
Since C++11 the `std::async()` function has allowed independent tasks to to be scheduled to run asynchronously. The standard allows for an implementation to use an appropriate threading model but suggests (but does not require) that implementations provide a thread pool that has a number of threads that work for underlying hardware.

Thus the use of `std::async()` in this application is basically creating jobs in a queue that are run by the thread queue. Because each Job object is using a IThorStream to read from a socket when the stream blocks the thread is released to processes another Job and will be rescheduled when data has been written into the stream buffer.

