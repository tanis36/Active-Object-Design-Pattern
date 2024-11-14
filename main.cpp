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
        // Simulate delay
        this_thread::sleep_for(chrono::seconds(1));
        // Return the result of the process
        return data + 5;
    }
};

int main() {

}
