/*
    Name 1: Muhammad Moiz Ansari
    Roll# 1: 23i-0523
    Name 2: Faizan-ur-Rehman Khan
    Roll #2: 23i-3021
    
    Section: BS(CS)-F
    Instructor: Ms. Rabail Zahid
    OS Project - Module 1
*/

#include <iostream>
#include <unistd.h>
#include <queue>
#include <string.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <mutex>
using namespace std;

string airline_types[] = {"Commercial", "Military", "Medical", "Cargo"},
       aircraft_types[] = {"Commercial", "Cargo", "Emergency"},
       flight_status[] = {"Arrival", "Departure", "Emergency"},
       arrival_phases[] = {"Holding Phase", "Approach Phase", "Landing Phase", "Taxi Phase", "At Gate"},
       departure_phases[] = {"At Gate", "Taxi Phase", "Takeoff Roll", "Climb"};

int flight_id_assign = 1; // Variable for assigning flight ids
int no_of_airlines = 6;

void spaces(string str, int s, int limit = 99)
{
    int size = s - str.size();
    if (size > limit)
        size = limit;
    for (int i = 0; i < size; ++i)
        cout << " ";
}

//////////////////////////////
//                          //
//         AIRCRAFT         //
//                          //
//////////////////////////////

class Aircraft
{
public:
    int id;
    // airline_id;
    string type;
    float speed;
    bool isFaulty;

    Aircraft(int ID, string t = "", float sp = 0, bool faulty = 0)
    {
        id = ID;
        // airline_id = airID;
        type = t;
        speed = sp;
        isFaulty = faulty;
    }
};

/////////////////////////////
//                         //
//         AIRLINE         //
//                         //
/////////////////////////////

class Airline
{
public:
    // Basic
    string name,
        type;
    int num_aircrafts,
        num_flights;

    vector<Aircraft> aircrafts;
    int total_voilations;

    Airline(string n = "", string t = "", int no_air = 0, int f = 0)
    {
        name = n;
        type = t;
        num_aircrafts = no_air;
        num_flights = f;

        total_voilations = 0;

        for (int i = 1; i <= num_aircrafts; ++i)
        {
            if (type == airline_types[1] || type == airline_types[2])
                t = aircraft_types[2];
            aircrafts.emplace_back(i, t);
        }
    }
};

vector<Airline> airlines_vec;
Aircraft *aircraft_assign_ptr;

////////////////////////////
//                        //
//         FLIGHT         //
//                        //
////////////////////////////

class Flight
{
public:
    int id,
        priority;
    string direction,
        status,
        phase;
    float speed;
    bool AVN_status;
    Aircraft *aircrafts;

    Flight(string d, string st = "", string ph = "", float s = 0, int pr = 0, bool avns = 0)
    {
        id = flight_id_assign++;
        direction = d;
        status = st;
        if (ph == "")
        {
            if (status == flight_status[0]) // Arrival
                ph = arrival_phases[0];
            else
                ph = departure_phases[0];
        }
        phase = ph;
        speed = s;
        priority = pr;
        AVN_status = avns;

        this->aircrafts = aircraft_assign_ptr;
        aircraft_assign_ptr += sizeof(Aircraft);
        aircraft_assign_ptr->speed = speed;
    }
};

///////////////////////////////////////////////
//                                           //
//                  RUNWAY                   //
//                                           //
///////////////////////////////////////////////

class Runway
{
public:
    string name;
    bool isOccupied;
    mutex rMutex;

    Runway(string n)
    {
        name = n;
        isOccupied = false;
    }

    bool useRunway()
    {
        rMutex.lock();

        if (isOccupied == false)
        {
            isOccupied = true;
            rMutex.unlock();
            return true;
        }

        rMutex.unlock();
        return false;
    }

    void freeRunway()
    {
        rMutex.lock();
        isOccupied = false;
        rMutex.unlock();
    }
};

///////////////////////////////////////////////
//                                           //
//               ATC LOGIC                   //
//                                           //
///////////////////////////////////////////////

vector<Runway *> runways; // Handles Runways

vector<Flight *> arrQ; // Arrival Queue
vector<Flight *> depQ; // Departure Queue
mutex arrMutex;        // Mutex for ArrQ
mutex depMutex;        // Mutex for DepQ

vector<pthread_t> pIds;
bool simRunning; // Global Boolean to Show if Simulation is running

void *dispatchArrivalFlights(void *arg)
{
    auto northTime = chrono::high_resolution_clock::now();
    auto southTime = chrono::high_resolution_clock::now();
    while (simRunning)
    {
        auto now = chrono::high_resolution_clock::now();
        auto northDuration = chrono::duration_cast<chrono::seconds>(now - northTime);
        auto southDuration = chrono::duration_cast<chrono::seconds>(now - southTime);

        if (northDuration.count() >= 180)
        {
            // North Flight to be Added

            northTime = chrono::high_resolution_clock::now();
        }

        if (southDuration.count() >= 120)
        {
            // South Flight to be Added

            southTime = chrono::high_resolution_clock::now();
        }
    }

    cout << ">> Arrival Flight Dispatcher is ending." << endl;

    pthread_exit(NULL);
}

void *dispatchDepartureFlights(void *arg)
{
    auto eastTime = chrono::high_resolution_clock::now();
    auto westTime = chrono::high_resolution_clock::now();
    while (simRunning)
    {
        auto now = chrono::high_resolution_clock::now();
        auto eastDuration = chrono::duration_cast<chrono::seconds>(now - eastTime);
        auto westDuration = chrono::duration_cast<chrono::seconds>(now - westTime);

        if (eastDuration.count() >= 150)
        {
            // East Flight to be Added

            eastTime = chrono::high_resolution_clock::now();
        }

        if (westDuration.count() >= 240)
        {
            // West Flight to be Added

            westTime = chrono::high_resolution_clock::now();
        }
    }

    cout << ">> Departure Flight Dispatcher is ending." << endl;

    pthread_exit(NULL);
}

Flight *highestPriorityFlight(vector<Flight *> q)
{
    // hP is for the highest priority flight
    Flight *hP = q[0];
    for (auto x : q)
    {
        if (x->priority > hP->priority)
            hP = x;
    }

    return hP;
}

void removeFlightFromQ(vector<Flight *> q, Flight *r)
{
    int index = 0;
    for (auto x : q)
    {
        if (x == r)
            break;
        index++;
    }

    q.erase(q.begin() + index);
}

void *statusTracker(void *arg)
{
    // Implement the Status Tracking Logic

    pthread_exit(NULL);
}

void *handleArrivals(void *arg)
{
    while (simRunning)
    {
        // Obtaining the Highest Priority Arrival
        Flight *toLand = nullptr;
        arrMutex.lock();
        if (!arrQ.empty())
        {
            toLand = highestPriorityFlight(arrQ);
            removeFlightFromQ(arrQ, toLand);
        }
        arrMutex.unlock();

        // Obtaining Available Runway
        Runway *toLandOn = nullptr;
        if (runways[0]->useRunway())
        {
            toLandOn = runways[0];
        }
        // In Case of Overflow
        else if (runways[2]->useRunway())
        {
            toLandOn = runways[2];
        }

        if (toLand && toLandOn)
        {
            cout << ">>Runway " << toLandOn->name << " being used for Flight " << toLand->id << endl;
            // Start the flight Status Tracking Thread
        }
    }
    pthread_exit(NULL);
}

void *handleDepartures(void *arg)
{
    while (simRunning)
    {
        while (simRunning)
        {
            // Obtaining the Highest Priority Arrival
            Flight *toLand = nullptr;
            depMutex.lock();
            if (!depQ.empty())
            {
                toLand = highestPriorityFlight(depQ);
                removeFlightFromQ(depQ, toLand);
            }
            depMutex.unlock();

            // Obtaining Available Runway
            Runway *toLandOn = nullptr;
            if (runways[0]->useRunway())
            {
                toLandOn = runways[0];
            }
            // In Case of Overflow
            else if (runways[2]->useRunway())
            {
                toLandOn = runways[2];
            }

            if (toLand && toLandOn)
            {
                cout << ">>Runway " << toLandOn->name << " being used for Flight " << toLand->id << endl;
                // Start the flight Status Tracking Thread
            }
        }
    }
    pthread_exit(NULL);
}

void StartSimulation()
{
    simRunning = true;

    runways.emplace_back(new Runway("RWY-A"));
    runways.emplace_back(new Runway("RWY-B"));
    runways.emplace_back(new Runway("RWY-C"));

    pIds.emplace_back();
    pthread_create(&(pIds.back()), NULL, dispatchArrivalFlights, NULL);
    pIds.emplace_back();
    pthread_create(&(pIds.back()), NULL, dispatchDepartureFlights, NULL);
    pIds.emplace_back();
    pthread_create(&(pIds.back()), NULL, handleArrivals, NULL);
    pIds.emplace_back();
    pthread_create(&(pIds.back()), NULL, handleDepartures, NULL);
}

/////////////////////////////////////////////
//                                         //
//               FUNCTIONS                 //
//                                         //
/////////////////////////////////////////////

void parse_airlines_CSV(vector<Airline> &airlines, string filename)
{
    ifstream file(filename);

    string line, word;
    getline(file, line); // Discard header

    int i = 0;
    while (getline(file, line))
    {
        stringstream ss(line); // To read each word by splitting string with ','
        vector<string> row;

        while (getline(ss, word, ','))
            row.push_back(word);

        if (row.size() == 4)
        {
            airlines.emplace_back(row[0], row[1], stoi(row[2]), stoi(row[3]));
        }
    }
}

void parse_flights_CSV(vector<Flight> &flights, vector<Airline> &airlines, string filename)
{
    ifstream file(filename);

    string line, word;
    getline(file, line); // Discard header

    int num_of_rows = 0;
    for (int i = 0; i < airlines.size(); ++i)
        num_of_rows += airlines[i].num_aircrafts;

    int i = 0;
    // while (getline(file, line))
    for (int j = 0; j < num_of_rows; ++j)
    {
        getline(file, line);
        stringstream ss(line); // To read each word by splitting string with ','
        vector<string> row;

        while (getline(ss, word, ','))
            row.push_back(word);

        if (row.size() == 6)
        {
            flights.emplace_back(row[0], row[1], row[2], stof(row[3]), stoi(row[4]), stoi(row[5]));
        }
    }
}

void display_airlines(vector<Airline> vec)
{
    int space[] = {25, 15, 19, 20};
    cout << "\n\033[1mName";
    spaces("Name", space[0]);
    cout << "Type";
    spaces("Type", space[1]);
    cout << "No. of Aircrafts";
    spaces("No. of Aircrafts", space[2]);
    cout << "No. of Voilations\n\033[0m";

    for (int i = 0; i < vec.size(); ++i)
    {
        cout << vec[i].name;
        spaces(vec[i].name, space[0]);
        cout << vec[i].type;
        spaces(vec[i].type, space[1]);
        cout << vec[i].num_aircrafts;
        spaces(to_string(vec[i].num_aircrafts), space[2]);
        cout << vec[i].num_flights << endl;
    }
    cout << endl;
}

void display_flights(vector<Flight> vec)
{
    int space[] = {12, 12, 12, 16, 12};
    string str;

    cout << "\n\033[1m";
    str = "Flight ID";
    cout << str;
    spaces(str, space[0]);
    str = "Direction";
    cout << str;
    spaces(str, space[1]);
    str = "Status";
    cout << str;
    spaces(str, space[2]);
    str = "Phase";
    cout << str;
    spaces(str, space[3]);
    str = "Speed";
    cout << str;
    spaces(str, space[4]);
    cout << "AVN Status\n\033[0m";

    for (int i = 0; i < vec.size(); ++i)
    {
        cout << vec[i].id;
        spaces(to_string(vec[i].id), space[0]);
        cout << vec[i].direction;
        spaces(vec[i].direction, space[1]);
        cout << vec[i].status;
        spaces(vec[i].status, space[2]);
        cout << vec[i].phase;
        spaces(vec[i].phase, space[3]);
        cout << to_string(vec[i].speed);
        spaces(to_string(vec[i].speed), space[4], 9);
        cout << vec[i].AVN_status << endl;
    }
    cout << endl;
}

int main()
{
    //////////////////// Input ////////////////////
    string filename = "airlines_data.csv";
    parse_airlines_CSV(airlines_vec, filename);
    aircraft_assign_ptr = &(airlines_vec[0].aircrafts[0]);
    filename = "flights_data.csv";
    vector<Airline> &airlines_arr = airlines_vec; // To be accessed in debugger
    vector<Flight> flights_arr;
    parse_flights_CSV(flights_arr, airlines_arr, filename);

    // Display data
    display_airlines(airlines_arr);
    display_flights(flights_arr);

    // Starts the Simulation, initializes variables
    // StartSimulation();

    return 0;
}