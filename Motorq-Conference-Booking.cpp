#include <string>

#include <vector>

#include <map>

#include <queue>

#include <ctime>

#include <stdexcept>

#include <mutex>
#include <thread>

// Forward declarations
class Conference;
class User;
class Booking;

// Helper struct for timestamps
struct Timestamp {
    time_t time;

    bool operator < (const Timestamp & other) const {
        return time < other.time;
    }
};

class Conference {
    private: string name;
    string location;
    vector < string > topics;
    Timestamp startTime;
    Timestamp endTime;
    int totalSlots;
    int availableSlots;

    public: Conference(const string & name,
        const string & location,
            const vector < string > & topics,
                const Timestamp & start,
                    const Timestamp & end, int slots);

    public: bool decreaseAvailableSlots();
    void increaseAvailableSlots();
    int getAvailableSlots() const {
        return availableSlots;
    }
    bool hasStarted() const;
    Timestamp getStartTime() const {
        return startTime;
    }
    Timestamp getEndTime() const {
        return endTime;
    }

    bool hasSlotAvailable() const;
    bool isTimeOverlapping(const Timestamp & start,
        const Timestamp & end) const;
    const string & getName() const;
    // Add other getters as needed
};

bool Conference::decreaseAvailableSlots() {
    if (availableSlots > 0) {
        availableSlots--;
        return true;
    }
    return false;
}

void Conference::increaseAvailableSlots() {
    if (availableSlots < totalSlots) {
        availableSlots++;
    }
}

bool Conference::hasStarted() const {
    return time(nullptr) >= startTime.time;
}

Conference::Conference(const string & _name,
    const string & _location,
        const vector < string > & _topics,
            const Timestamp & start,
                const Timestamp & end, int slots) {
    if (_topics.size() > 10) {
        throw runtime_error("Maximum 10 topics allowed");
    }
    if (slots <= 0) {
        throw runtime_error("Slots must be greater than 0");
    }
    if (end.time - start.time > 12 * 3600) {
        throw runtime_error("Conference duration cannot exceed 12 hours");
    }
    if (start.time >= end.time) {
        throw runtime_error("Start time must be before end time");
    }

    name = _name;
    location = _location;
    topics = _topics;
    startTime = start;
    endTime = end;
    totalSlots = slots;
    availableSlots = slots;
}

bool Conference::hasSlotAvailable() const {
    return availableSlots > 0;
}

bool Conference::isTimeOverlapping(const Timestamp & start,
    const Timestamp & end) const {
    return !(end.time <= startTime.time || start.time >= endTime.time);
}

const string & Conference::getName() const {
    return name;
}

enum class BookingStatus {
    CONFIRMED,
    WAITLISTED,
    CANCELED
};

class User {
    private: string userId;
    vector < string > interestedTopics;
    map < string,
    BookingStatus > bookingStatuses; // bookingId -> status

    public: User(const string & id,
        const vector < string > & topics) {
        if (topics.size() > 50) {
            throw runtime_error("Maximum 50 interested topics allowed");
        }
        userId = id;
        interestedTopics = topics;
    }

    void addBooking(const string & bookingId, BookingStatus status) {
        bookingStatuses[bookingId] = status;
    }

    void updateBookingStatus(const string & bookingId, BookingStatus status) {
        if (bookingStatuses.find(bookingId) != bookingStatuses.end()) {
            bookingStatuses[bookingId] = status;
        }
    }

    void removeBooking(const string& bookingId) {
        auto it = bookingStatuses.find(bookingId);
        if (it != bookingStatuses.end()) {
            bookingStatuses.erase(it);
        }
    }

    vector < string > getActiveBookings() const {
        vector < string > active;
        for (const auto & [id, status]: bookingStatuses) {
            if (status != BookingStatus::CANCELED) {
                active.push_back(id);
            }
        }
        return active;
    }

    const string & getUserId() const {
        return userId;
    }
};

// bool User::hasConflictingBooking(const Conference& conf) const {
//     // For basic implementation, we'll skip the actual conflict checking
//     // In real implementation, we would check each active booking's conference time
//     return false;  // Placeholder for now
// }

ostream & operator << (ostream & os,
    const BookingStatus & status) {
    switch (status) {
    case BookingStatus::CONFIRMED:
        os << "CONFIRMED";
        break;
    case BookingStatus::WAITLISTED:
        os << "WAITLISTED";
        break;
    case BookingStatus::CANCELED:
        os << "CANCELED";
        break;
    }
    return os;
}

class Booking {
    private: string bookingId;
    string userId;
    string conferenceName;
    BookingStatus status;
    Timestamp confirmationDeadline; // for waitlisted bookings

    public: Booking(): status(BookingStatus::CANCELED) {}
    Booking(const string & userId,
        const string & confName);
    const string & getBookingId() const;
    const string & getUserId() const {
        return userId;
    }
    const string & getConferenceName() const {
        return conferenceName;
    }
    BookingStatus getStatus() const;
    void setStatus(BookingStatus newStatus);
    void setConfirmationDeadline(const Timestamp & deadline) {
        confirmationDeadline = deadline;
    }
    Timestamp getConfirmationDeadline() const {
        return confirmationDeadline;
    }
};

Booking::Booking(const string & _userId,
    const string & confName) {
    userId = _userId;
    conferenceName = confName;
    status = BookingStatus::CONFIRMED; // Default status
    // We'll generate proper ID later
    bookingId = _userId + "_" + confName + "_" + to_string(time(nullptr));
}

const string & Booking::getBookingId() const {
    return bookingId;
}

BookingStatus Booking::getStatus() const {
    return status;
}

void Booking::setStatus(BookingStatus newStatus) {
    status = newStatus;
}

class ConferenceBookingSystem {
    private: map < string,
    Conference > conferences; // name -> Conference
    map < string,
    User > users; // userId -> User
    map < string,
    Booking > bookings; // bookingId -> Booking
    map < string,
    queue < string >> waitlists; // conferenceName -> queue of bookingIds

    // Add mutexes for protecting shared resources
    mutable mutex conference_mutex;
    mutable mutex booking_mutex;
    mutable mutex waitlist_mutex;

        // Helper method to perform atomic booking operation
    string atomicBookingOperation(const string& userId, const string& conferenceName) {
        lock_guard<mutex> conf_lock(conference_mutex);
        lock_guard<mutex> book_lock(booking_mutex);
        lock_guard<mutex> wait_lock(waitlist_mutex);
        
        auto& conference = conferences.at(conferenceName);
        
        // Check if conference has started
        if (conference.hasStarted()) {
            throw runtime_error("Cannot book conference that has already started");
        }
        
        // Check for existing booking
        for (const auto& [id, booking] : bookings) {
            if (booking.getUserId() == userId && 
                booking.getConferenceName() == conferenceName &&
                booking.getStatus() != BookingStatus::CANCELED) {
                throw runtime_error("User already has an active booking for this conference with ID: " + id);
            }
        }
        
        // Check for conflicts
        if (hasConflictingBooking(userId, conference)) {
            throw runtime_error("User has a conflicting booking");
        }
        
        // Create booking
        Booking newBooking(userId, conferenceName);
        string bookingId = newBooking.getBookingId();
        
        // Try to get a slot
        if (conference.decreaseAvailableSlots()) {
            newBooking.setStatus(BookingStatus::CONFIRMED);
            removeFromOverlappingWaitlists(userId, conference);
        } else {
            newBooking.setStatus(BookingStatus::WAITLISTED);
            waitlists[conferenceName].push(bookingId);
        }
        
        bookings.emplace(bookingId, newBooking);
        users.at(userId).addBooking(bookingId, newBooking.getStatus());
        
        return bookingId;
    }


    public:
        // Conference management
        void addConference(const string & name,
            const string & location,
                const vector < string > & topics,
                    const Timestamp & start,
                        const Timestamp & end, int slots);

    // User management
    void addUser(const string & userId,
        const vector < string > & topics);

    // Booking operations
    string bookConference(const string & userId,
        const string & conferenceName);
    bool confirmWaitlistedBooking(const string & bookingId);
    bool cancelBooking(const string & bookingId);
    BookingStatus getBookingStatus(const string & bookingId) const;

    private: void processWaitlist(const string & conferenceName);
    string generateBookingId() const;
    void validateConferenceExists(const string & name) const;
    void validateUserExists(const string & userId) const;
    void setWaitlistConfirmationDeadline(Booking & booking) {
        Timestamp deadline {
            time(nullptr) + 3600
        }; // 1 hour from now
        booking.setConfirmationDeadline(deadline);
        cout << "Set confirmation deadline to: " << ctime( & deadline.time);
    }
    bool hasConflictingBooking(const string & userId,
        const Conference & newConference) {
        cout << "Checking for conflicting bookings for user: " << userId << endl;

        const User & user = users.at(userId);
        auto activeBookings = user.getActiveBookings();

        for (const string & bookingId: activeBookings) {
            const Booking & booking = bookings.at(bookingId);
            if (booking.getStatus() == BookingStatus::CANCELED) continue;
            if (booking.getStatus() != BookingStatus::CONFIRMED) continue;

            const Conference & existingConf = conferences.at(booking.getConferenceName());
            if (existingConf.isTimeOverlapping(newConference.getStartTime(),
                    newConference.getEndTime())) {
                cout << "Found conflicting booking: " << bookingId << endl;
                return true;
            }
        }
        return false;
    }

    void removeFromOverlappingWaitlists(const string & userId,
        const Conference & bookedConf) {
        cout << "Removing user from overlapping conference waitlists" << endl;

        vector < string > conferencesToUpdate;

        // Find all overlapping conferences
        for (const auto & [confName, conf]: conferences) {
            if (confName != bookedConf.getName() &&
                conf.isTimeOverlapping(bookedConf.getStartTime(), bookedConf.getEndTime())) {
                conferencesToUpdate.push_back(confName);
            }
        }

        // Remove from their waitlists
        for (const string & confName: conferencesToUpdate) {
            auto & waitlist = waitlists[confName];
            queue < string > updatedQueue;

            while (!waitlist.empty()) {
                string bookingId = waitlist.front();
                waitlist.pop();

                const Booking & booking = bookings.at(bookingId);
                if (booking.getUserId() != userId) {
                    updatedQueue.push(bookingId);
                } else {
                    // Cancel the waitlisted booking
                    bookings.at(bookingId).setStatus(BookingStatus::CANCELED);
                    cout << "Canceled overlapping waitlisted booking: " << bookingId << endl;
                }
            }

            waitlists[confName] = updatedQueue;
        }
    }

    void cancelAllWaitlistedBookings(const string & conferenceName) {
        cout << "Canceling all waitlisted bookings for conference: " << conferenceName << endl;

        auto & waitlist = waitlists[conferenceName];
        while (!waitlist.empty()) {
            string bookingId = waitlist.front();
            waitlist.pop();

            auto & booking = bookings.at(bookingId);
            booking.setStatus(BookingStatus::CANCELED);
            cout << "Canceled waitlisted booking: " << bookingId << endl;
        }
    }

};

bool ConferenceBookingSystem::cancelBooking(const string & bookingId) {
    lock_guard<mutex> conf_lock(conference_mutex);
    lock_guard<mutex> book_lock(booking_mutex);
    lock_guard<mutex> wait_lock(waitlist_mutex);

    cout << "Attempting to cancel booking: " << bookingId << endl;

    auto bookingIt = bookings.find(bookingId);
    if (bookingIt == bookings.end()) {
        throw runtime_error("Booking not found");
    }

    Booking & booking = bookingIt -> second;
    if (booking.getStatus() == BookingStatus::CANCELED) {
        throw runtime_error("Booking is already canceled");
    }

    // Check if conference has started
    Conference & conference = conferences.at(booking.getConferenceName());
    if (conference.hasStarted()) {
        throw runtime_error("Cannot cancel booking after conference has started");
    }

    // If it was a confirmed booking, increase available slots
    if (booking.getStatus() == BookingStatus::CONFIRMED) {
        conference.increaseAvailableSlots();
        cout << "Slot freed up. Available slots: " << conference.getAvailableSlots() << endl;

        // Process waitlist if any
        processWaitlist(booking.getConferenceName());
    } else if (booking.getStatus() == BookingStatus::WAITLISTED) {
        // Remove from waitlist queue
        auto & waitlist = waitlists[booking.getConferenceName()];
        queue < string > updatedQueue;
        while (!waitlist.empty()) {
            string currentId = waitlist.front();
            waitlist.pop();
            if (currentId != bookingId) {
                updatedQueue.push(currentId);
            }
        }
        waitlists[booking.getConferenceName()] = updatedQueue;
        cout << "Removed from waitlist" << endl;
    }

    booking.setStatus(BookingStatus::CANCELED);
    users.at(booking.getUserId()).removeBooking(bookingId);
    cout << "Booking canceled successfully" << endl;
    return true;
}

void ConferenceBookingSystem::processWaitlist(const string & conferenceName) {
    cout << "Processing waitlist for conference: " << conferenceName << endl;

    auto & waitlist = waitlists[conferenceName];
    if (waitlist.empty()) {
        cout << "No users in waitlist" << endl;
        return;
    }

    // Get next waitlisted booking
    string nextBookingId = waitlist.front();
    auto & booking = bookings.at(nextBookingId);

    // Set confirmation deadline
    setWaitlistConfirmationDeadline(booking);
    cout << "Notifying user " << booking.getUserId() << " about available slot" << endl;
}

void ConferenceBookingSystem::addConference(const string & name,
    const string & location,
        const vector < string > & topics,
            const Timestamp & start,
                const Timestamp & end, int slots) {
    if (conferences.find(name) != conferences.end()) {
        throw runtime_error("Conference with this name already exists");
    }

    conferences.emplace(name, Conference(name, location, topics, start, end, slots));
}

void ConferenceBookingSystem::addUser(const string & userId,
    const vector < string > & topics) {
    if (users.find(userId) != users.end()) {
        throw runtime_error("User already exists");
    }

    users.emplace(userId, User(userId, topics));
}

string ConferenceBookingSystem::bookConference(const string & userId,
    const string & conferenceName) {
    cout << "Attempting to book conference: " << conferenceName << " for user: " << userId << endl;

    validateUserExists(userId);
    validateConferenceExists(conferenceName);

    // Check if conference has started
    auto & conference = conferences.at(conferenceName);
    if (conference.hasStarted()) {
        throw runtime_error("Cannot book conference that has already started");
    }

    // Check for existing booking
    for (const auto & [id, booking]: bookings) {
        if (booking.getUserId() == userId &&
            booking.getConferenceName() == conferenceName &&
            booking.getStatus() != BookingStatus::CANCELED) {
            throw runtime_error("User already has an active booking for this conference with ID: " + id);
        }
    }

    // Check for conflicts
    if (hasConflictingBooking(userId, conference)) {
        throw runtime_error("User has a conflicting booking");
    }

    // Create booking
    Booking newBooking(userId, conferenceName);
    string bookingId = newBooking.getBookingId();

    // Try to get a slot
    if (conference.decreaseAvailableSlots()) {
        cout << "Slot available. Creating confirmed booking." << endl;
        newBooking.setStatus(BookingStatus::CONFIRMED);
        // Remove from overlapping waitlists
        removeFromOverlappingWaitlists(userId, conference);
    } else {
        cout << "No slots available. Adding to waitlist." << endl;
        newBooking.setStatus(BookingStatus::WAITLISTED);
        // Add to waitlist
        waitlists[conferenceName].push(bookingId);
    }

    bookings.emplace(bookingId, newBooking);
    
    // Add the booking to the user's bookingStatuses map
    users.at(userId).addBooking(bookingId, newBooking.getStatus());

    
    // const User & user = users.at(userId);
    //     auto activeBookings = user.getActiveBookings();
    // cout << userId << " " << bookingId << " " << activeBookings.size() << "\n";
    
    cout << "Booking created with ID: " << bookingId << endl;

    return bookingId;
}

BookingStatus ConferenceBookingSystem::getBookingStatus(const string & bookingId) const {
    cout << "Checking status for booking: " << bookingId << endl;

    auto it = bookings.find(bookingId);
    if (it == bookings.end()) {
        throw runtime_error("Booking not found");
    }

    const Booking & booking = it -> second;
    BookingStatus status = booking.getStatus();

    cout << "Status: " << status << endl;
    if (status == BookingStatus::WAITLISTED) {
        const auto & conference = conferences.at(booking.getConferenceName());
        if (conference.hasSlotAvailable()) {
            Timestamp deadline = booking.getConfirmationDeadline();
            cout << "Slot is available for confirmation until: " <<
                ctime( & deadline.time);
        }
    }

    return status;
}

void ConferenceBookingSystem::validateConferenceExists(const string & name) const {
    if (conferences.find(name) == conferences.end()) {
        throw runtime_error("Conference not found");
    }
}

void ConferenceBookingSystem::validateUserExists(const string & userId) const {
    if (users.find(userId) == users.end()) {
        throw runtime_error("User not found");
    }
}

string ConferenceBookingSystem::generateBookingId() const {
    // Simple implementation for now
    return to_string(time(nullptr)) + "_" + to_string(rand());
}

bool ConferenceBookingSystem::confirmWaitlistedBooking(const string & bookingId) {
    lock_guard<mutex> conf_lock(conference_mutex);
    lock_guard<mutex> book_lock(booking_mutex);
    lock_guard<mutex> wait_lock(waitlist_mutex);
    cout << "Attempting to confirm waitlisted booking: " << bookingId << endl;

    auto bookingIt = bookings.find(bookingId);
    if (bookingIt == bookings.end()) {
        throw runtime_error("Booking not found");
    }

    Booking & booking = bookingIt -> second;
    if (booking.getStatus() != BookingStatus::WAITLISTED) {
        throw runtime_error("Booking is not in waitlisted state");
    }

    const string & conferenceName = booking.getConferenceName();
    Conference & conference = conferences.at(conferenceName);

    // Check if conference has started
    if (conference.hasStarted()) {
        cancelAllWaitlistedBookings(conferenceName);
        throw runtime_error("Cannot confirm booking after conference has started");
    }

    // Check if confirmation deadline has passed
    if (time(nullptr) > booking.getConfirmationDeadline().time) {
        cout << "Confirmation deadline has passed" << endl;
        // Move to end of waitlist
        auto & waitlist = waitlists[conferenceName];
        queue < string > updatedQueue;
        bool foundCurrent = false;

        // Remove from current position
        while (!waitlist.empty()) {
            string currentId = waitlist.front();
            waitlist.pop();
            if (currentId != bookingId) {
                updatedQueue.push(currentId);
            } else {
                foundCurrent = true;
            }
        }

        // Add to end if it was found
        if (foundCurrent) {
            updatedQueue.push(bookingId);
        }

        waitlists[conferenceName] = updatedQueue;

        // Process next in waitlist if slot is still available
        if (conference.hasSlotAvailable()) {
            processWaitlist(conferenceName);
        }

        return false;
    }

    if (hasConflictingBooking(booking.getUserId(), conference)) {
        throw runtime_error("User now has a conflicting booking");
    }

    // Check if slot is still available
    if (!conference.decreaseAvailableSlots()) {
        throw runtime_error("No slots available");
    }

    // Remove from waitlist
    auto & waitlist = waitlists[conferenceName];
    queue < string > updatedQueue;
    while (!waitlist.empty()) {
        string currentId = waitlist.front();
        waitlist.pop();
        if (currentId != bookingId) {
            updatedQueue.push(currentId);
        }
    }
    waitlists[conferenceName] = updatedQueue;

    // Confirm the booking
    booking.setStatus(BookingStatus::CONFIRMED);
    cout << "Booking confirmed successfully" << endl;

    // Remove from overlapping waitlists
    removeFromOverlappingWaitlists(booking.getUserId(), conference);

    return true;
}

// int main() {
//     ConferenceBookingSystem system;

//     try {
//         // Test adding a conference with just 1 slot
//         vector < string > confTopics = {
//             "C++",
//             "Programming"
//         };
//         Timestamp startTime {
//             time(nullptr) + 3600
//         }; // Start 1 hour from now
//         Timestamp endTime {
//             time(nullptr) + 3600 * 3
//         }; // 3 hours from now

//         system.addConference("CPP Conference", "New York", confTopics, startTime, endTime, 1);
//         cout << "\nConference added successfully\n";

//         // Add two users
//         vector < string > userTopics = {
//             "C++",
//             "Software"
//         };
//         system.addUser("user123", userTopics);
//         system.addUser("user456", userTopics);
//         cout << "Users added successfully\n";

//         // Test booking for first user
//         string booking1 = system.bookConference("user123", "CPP Conference");
//         cout << "First booking status: " << system.getBookingStatus(booking1) << "\n";

//         // Test booking for second user (should go to waitlist)
//         string booking2 = system.bookConference("user456", "CPP Conference");
//         cout << "Second booking status: " << system.getBookingStatus(booking2) << "\n";

//         // Test canceling first booking
//         cout << "\nCanceling first booking...\n";
//         system.cancelBooking(booking1);

//         // Check status of both bookings
//         cout << "\nChecking booking statuses after cancellation:\n";
//         cout << "First booking status: " << system.getBookingStatus(booking1) << "\n";
//         cout << "Second booking status: " << system.getBookingStatus(booking2) << "\n";

//         // Test confirming the waitlisted booking
//         cout << "\nConfirming second booking...\n";
//         system.confirmWaitlistedBooking(booking2);

//         // Check final status
//         cout << "\nFinal status of second booking:\n";
//         cout << "Second booking status: " << system.getBookingStatus(booking2) << "\n";

//     } catch (const exception & e) {
//         cerr << "Error: " << e.what() << endl;
//         return 1;
//     }

//     return 0;
// }

// int main() {
//     ConferenceBookingSystem system;

//     try {
//         // Add two overlapping conferences
//         vector < string > confTopics = {
//             "C++",
//             "Programming"
//         };
//         Timestamp startTime1 {
//             time(nullptr) + 3600
//         }; // Start 1 hour from now
//         Timestamp endTime1 {
//             time(nullptr) + 3600 * 3
//         }; // 3 hours duration

//         Timestamp startTime2 {
//             time(nullptr) + 3600 * 2
//         }; // Start 2 hours from now
//         Timestamp endTime2 {
//             time(nullptr) + 3600 * 4
//         }; // 3 hours duration

//         system.addConference("CPP Conference", "New York", confTopics, startTime1, endTime1, 1);
//         system.addConference("Java Conference", "New York", confTopics, startTime2, endTime2, 1);
//         cout << "Conferences added successfully\n";

//         // Add user
//         vector < string > userTopics = {
//             "C++",
//             "Software"
//         };
//         system.addUser("user123", userTopics);
//         cout << "User added successfully\n";

//         // Book first conference
//         string booking1 = system.bookConference("user123", "CPP Conference");
//         cout << "First booking status: " << system.getBookingStatus(booking1) << "\n";

//         // Try to book overlapping conference (should fail)
//         try {
//             string booking2 = system.bookConference("user123", "Java Conference");
//         } catch (const runtime_error & e) {
//             cout << "Expected error when booking overlapping conference: " << e.what() << "\n";
//         }

//     } catch (const exception & e) {
//         cerr << "Error: " << e.what() << endl;
//         return 1;
//     }

//     return 0;
// }

// void test_concurrent_booking() {
//     ConferenceBookingSystem system;
    
//     // Setup conference
//     vector<string> confTopics = {"C++", "Programming"};
//     Timestamp startTime{time(nullptr) + 3600};
//     Timestamp endTime{time(nullptr) + 7200};
//     system.addConference("Concurrent Test Conf", "New York", confTopics, startTime, endTime, 2);
    
//     // Setup users
//     vector<string> userTopics = {"C++", "Software"};
//     for(int i = 1; i <= 5; i++) {
//         system.addUser("user" + to_string(i), userTopics);
//     }
    
//     // Create threads to simulate concurrent booking
//     vector<thread> threads;
//     vector<string> bookingIds(5);
//     vector<string> errors(5);
    
//     for(int i = 0; i < 5; i++) {
//         threads.emplace_back([&system, i, &bookingIds, &errors]() {
//             try {
//                 string userId = "user" + to_string(i + 1);
//                 bookingIds[i] = system.bookConference(userId, "Concurrent Test Conf");
//             } catch (const exception& e) {
//                 errors[i] = e.what();
//             }
//         });
//     }
    
//     // Wait for all threads to complete
//     for(auto& thread : threads) {
//         thread.join();
//     }
    
//     // Verify results
//     int confirmed = 0;
//     int waitlisted = 0;
    
//     cout << "\nBooking results:\n";
//     for(int i = 0; i < 5; i++) {
//         if(!bookingIds[i].empty()) {
//             BookingStatus status = system.getBookingStatus(bookingIds[i]);
//             cout << "User" << (i+1) << " booking status: " << status << "\n";
//             if(status == BookingStatus::CONFIRMED) confirmed++;
//             if(status == BookingStatus::WAITLISTED) waitlisted++;
//         } else if(!errors[i].empty()) {
//             cout << "User" << (i+1) << " error: " << errors[i] << "\n";
//         }
//     }
    
//     cout << "\nTotal confirmed: " << confirmed << " (should be 2)\n";
//     cout << "Total waitlisted: " << waitlisted << " (should be 3)\n";
// }

// int main() {
//     cout << "Running concurrent booking tests...\n";
//     test_concurrent_booking();
//     return 0;
// }
