#include <iostream>
#include <future>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

using namespace std;

// MethodRequest encapsulates a client request as a callable
using MethodRequest = function<void()>;

// Class Scheduler manages the queue of MethodRequests
class Scheduler {
public:
    // Enqueues a new MethodRequest
    void enqueue(MethodRequest request) {
        lock_guard<mutex> lock(mtx);
        requests.push(request);  // Add request to the queue
        cv.notify_one();  // Notify worker thread that a request is available
    }

    // Starts the scheduler and processes requests in a separate thread
    void start() {
        worker = thread([this]() {
            while (true) {
                MethodRequest request;
                {
                    unique_lock<mutex> lock(mtx);
                    // Wait until there is a request in the queue
                    cv.wait(lock, [this]() { return !requests.empty(); });
                    request = requests.front();
                    requests.pop();
                }
                // If there is a request, execute the request, otherwise break out of the loop
                if (request) {
                    request();
                } else {
                    break;
                }
            }
        });
    }

    // Stops the scheduler and joins the worker thread
    void stop() {
        // Signal worker to stop by enqueuing a nullptr
        enqueue(nullptr);
        // If the worker thread is joinable, join it
        if (worker.joinable()) {
            worker.join();
        }
    }

private:
    // Queue that holds pending requests
    queue<MethodRequest> requests;
    // Mutex for thread-safe queue access
    mutex mtx;
    // Condition variable for request notification
    condition_variable cv;
    // Background thread to process requests
    thread worker;
};

// Class Servant performs the work that clients request through the Active Object
class Servant {
public:
    // Processing function that adds 5 to the input after a delay
    int process(int data) {
        // Simulate delay of 5 seconds
        this_thread::sleep_for(chrono::seconds(5));
        // Return the result of the process
        return data + 5;
    }
};

// Class ActiveObject is a proxy that provides an asynchronous interface to the Servant
class ActiveObject {
public:
    // Constructor
    ActiveObject() {
        // Start the scheduler once initialized
        scheduler.start();
    }

    // Destructor
    ~ActiveObject() {
        // Stop the scheduler upon destruction
        scheduler.stop();
    }

    // Asynchronous interface for clients to process data
    future<int> asyncProcess(int data) {
        // Shared promise
        auto promise = make_shared<std::promise<int>>();
        // Get future
        auto future = promise->get_future();

        // Enqueue a MethodRequest that will run in the background
        scheduler.enqueue([this, data, promise]() {
            // Perform the work
            int result = servant.process(data);
            // Set the result in the promise
            promise->set_value(result);
        });

        // Return the future
        return future;
    }

private:
    // Scheduler to manage requests
    Scheduler scheduler;
    // Servant to perform work
    Servant servant;
};

// Main
int main() {
    // Create an ActiveObject
    ActiveObject activeObject;

    // Submit a request
    future<int> result = activeObject.asyncProcess(12);

    cout << "Request sent, performing other work..." << endl;

    // Get the result and print it to the console when it is ready
    cout << "Result: " << result.get() << endl;
}