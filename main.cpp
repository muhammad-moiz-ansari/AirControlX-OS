/*
    Name 1: Muhammad Moiz Ansari
    Roll# 1: 23i-0523
    Name 2: Faizan-ur-Rehman Khan
    Roll# 2: 23i-3021

    Section: BS(CS)-F
    Instructor: Ms. Rabail Zahid
    OS Project - Module 2
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
       departure_phases[] = {"At Gate", "Taxi Phase", "Takeoff Roll", "Climb", "Departure"};

int flight_id_assign = 1; // Variable for assigning flight ids
int no_of_airlines = 6;
mutex coutMut;

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

////////////////////////////
//                        //
//         FLIGHT         //
//                        //
////////////////////////////

class Flight
{
public:
    int id,
        priority; // Lower number --> Lower priority
    string direction,
        status,
        phase;
    float speed;
    bool AVN_status;
    Aircraft *aircraft;

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
        priority = pr;  // Actual priority is assigned later according to aircraft
        AVN_status = avns;
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
        // If the runway is not occupied
        if (isOccupied == false)
        {
            isOccupied = true;
            return true;
        }
        return false;
    }

    void freeRunway()
    {
        isOccupied = false;
    }
};

// In order to send Flight Data as well as Runway Data
// To the Track Flight Function
class FlightWrapper
{
public:
    Runway *toLandOn;
    Flight *toLand;

    FlightWrapper()
    {
        toLandOn = NULL;
        toLand = NULL;
    }

    FlightWrapper(Flight *f, Runway *r)
    {
        toLand = f;
        toLandOn = r;
    }
};

///////////////////////////////////////////////
//                                           //
//               ATC LOGIC                   //
//                                           //
///////////////////////////////////////////////

vector<Runway *> runways;                     // Handles Runways
vector<Airline> &airlines_arr = airlines_vec; // To be accessed in debugger
vector<Flight *> flights_arr;

vector<Flight *> arrQ; // Arrival Queue
vector<Flight *> depQ; // Departure Queue
mutex arrMutex;        // Mutex for ArrQ
mutex depMutex;        // Mutex for DepQ

vector<pthread_t> pIds;
bool simRunning; // Global Boolean to Show if Simulation is running

void deleteFlightFromFlightArr(Flight *f)
{
    int index = 0;
    for (auto x : flights_arr)
    {
        if (x == f)
            break;
        index++;
    }

    flights_arr.erase(flights_arr.begin() + index);
}

void *dispatchArrivalFlights(void *arg)
{
    auto northTime = chrono::high_resolution_clock::now();
    auto southTime = chrono::high_resolution_clock::now();
    while (simRunning)
    {
        auto now = chrono::high_resolution_clock::now();
        auto northDuration = chrono::duration_cast<chrono::seconds>(now - northTime);
        auto southDuration = chrono::duration_cast<chrono::seconds>(now - southTime);

        if (northDuration.count() >= 18)
        {
            // North Flight to be Added
            Flight *chosen = nullptr;
            for (auto x : flights_arr)
            {
                if (x->direction == "North")
                {
                    chosen = x;
                    break;
                }
            }

            if (chosen)
            {
                arrMutex.lock();
                deleteFlightFromFlightArr(chosen);
                arrQ.push_back(chosen);
                arrMutex.unlock();

                coutMut.lock();
                cout << ">> Flight " << chosen->id << " is added to Arrival Queue." << endl;
                coutMut.unlock();
            }

            northTime = chrono::high_resolution_clock::now();
        }

        if (southDuration.count() >= 12)
        {
            // South Flight to be Added
            Flight *chosen = nullptr;
            for (auto x : flights_arr)
            {
                if (x->direction == "South")
                {
                    chosen = x;
                    break;
                }
            }

            if (chosen)
            {
                arrMutex.lock();
                deleteFlightFromFlightArr(chosen);
                arrQ.push_back(chosen);
                arrMutex.unlock();

                coutMut.lock();
                cout << ">> Flight " << chosen->id << " is added to Arrival Queue." << endl;
                coutMut.unlock();
            }

            southTime = chrono::high_resolution_clock::now();
        }
    }

    coutMut.lock();
    cout << ">> Arrival Flight Dispatcher is ending." << endl;
    coutMut.unlock();

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

        if (eastDuration.count() >= 15)
        {
            // East Flight to be Added
            Flight *chosen = nullptr;
            for (auto x : flights_arr)
            {
                if (x->direction == "East")
                {
                    chosen = x;
                    break;
                }
            }

            if (chosen)
            {
                depMutex.lock();
                deleteFlightFromFlightArr(chosen);
                depQ.push_back(chosen);
                depMutex.unlock();

                coutMut.lock();
                cout << ">> Flight " << chosen->id << " is added to Departure Queue." << endl;
                coutMut.unlock();
            }

            eastTime = chrono::high_resolution_clock::now();
        }

        if (westDuration.count() >= 24)
        {
            // West Flight to be Added
            Flight *chosen = nullptr;
            for (auto x : flights_arr)
            {
                if (x->direction == "West")
                {
                    chosen = x;
                    break;
                }
            }

            if (chosen)
            {
                depMutex.lock();
                deleteFlightFromFlightArr(chosen);
                depQ.push_back(chosen);
                depMutex.unlock();

                coutMut.lock();
                cout << ">> Flight " << chosen->id << " is added to Departure Queue." << endl;
                coutMut.unlock();
            }

            westTime = chrono::high_resolution_clock::now();
        }
    }

    coutMut.lock();
    cout << ">> Departure Flight Dispatcher is ending." << endl;
    coutMut.unlock();

    pthread_exit(NULL);
}

Flight *highestPriorityFlight(vector<Flight *> &q)
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

void removeFlightFromQ(vector<Flight *> &q, Flight *r)
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

void *flightTrack(void *arg)
{
    FlightWrapper *fw = (FlightWrapper *)arg;

    Flight *flt = fw->toLand;
    Runway *rw = fw->toLandOn;

    // Arrival Handling
    if (flt->status == flight_status[0])
    {
        sleep(5);
        flt->phase = arrival_phases[0];
        flt->speed = 400 + (rand() % 210);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[0] << endl;
        coutMut.unlock();
        sleep(5);
        flt->phase = arrival_phases[1];
        flt->speed = 240 + (rand() % 60);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[1] << endl;
        coutMut.unlock();
        sleep(5);
        flt->phase = arrival_phases[2];
        flt->speed = 30 + (rand() % 220);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[2] << endl;
        coutMut.unlock();
        sleep(5);
        flt->phase = arrival_phases[3];
        flt->speed = 15 + (rand() % 20);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[3] << endl;
        coutMut.unlock();
        sleep(5);
        flt->phase = arrival_phases[4];
        flt->speed = 0 + (rand() % 10);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[4] << endl;
        coutMut.unlock();

        coutMut.lock();
        cout << ">> Flight " << flt->id << " has Landed." << endl;
        cout << ">> Runway " << rw->name << " is Freed Up." << endl;
        coutMut.unlock();
        rw->freeRunway();
    }
    // Departure Handling
    else if (flt->status == flight_status[1])
    {
        sleep(5);
        flt->phase = departure_phases[0];
        flt->speed = 0 + (rand() % 15);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[0] << endl;
        coutMut.unlock();
        sleep(5);
        flt->phase = departure_phases[1];
        flt->speed = 15 + (rand() % 20);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[1] << endl;
        coutMut.unlock();
        sleep(5);
        flt->phase = departure_phases[2];
        flt->speed = 0 + (rand() % 300);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[2] << endl;
        coutMut.unlock();
        sleep(5);
        flt->phase = departure_phases[3];
        flt->speed = 250 + (rand() % 220);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[3] << endl;
        coutMut.unlock();
        sleep(5);
        flt->phase = departure_phases[4];
        flt->speed = 800 + (rand() % 110);
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[4] << endl;
        coutMut.unlock();

        coutMut.lock();
        cout << ">> Flight " << flt->id << " has Departed." << endl;
        cout << ">> Runway " << rw->name << " is Freed Up." << endl;
        coutMut.unlock();
        rw->freeRunway();
    }

    delete flt; // Freeing the memory allocated for Flight
    delete fw;  // Freeing the memory allocated for FlightWrapper
    pthread_exit(NULL);
}

void *handleArrivals(void *arg)
{
    while (simRunning)
    {
        if (arrQ.empty())
            continue;

        // Obtaining the Highest Priority Arrival
        Flight *toLand = nullptr;
        arrMutex.lock();
        if (!arrQ.empty())
        {
            toLand = highestPriorityFlight(arrQ);
            removeFlightFromQ(arrQ, toLand);
            coutMut.lock();
            coutMut.unlock();
        }
        arrMutex.unlock();

        // Obtaining Available Runway
        Runway *toLandOn = nullptr;
        runways[0]->rMutex.lock();
        runways[2]->rMutex.lock();

        if (runways[0]->useRunway())
        {
            toLandOn = runways[0];
        }
        else if (runways[2]->useRunway())
        {
            toLandOn = runways[2];
        }

        runways[0]->rMutex.unlock();
        runways[2]->rMutex.unlock();

        if (toLand && toLandOn)
        {
            coutMut.lock();
            cout << ">> Runway " << toLandOn->name << " being used for Flight " << toLand->id << endl;
            coutMut.unlock();
            // Start the flight Status Tracking Thread
            pthread_t t;
            pthread_create(&t, NULL, flightTrack, new FlightWrapper(toLand, toLandOn));
            pthread_detach(t);
        }
    }
    pthread_exit(NULL);
}

void *handleDepartures(void *arg)
{
    while (simRunning)
    {
        if (depQ.empty())
            continue;

        // Obtaining the Highest Priority Departure
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
        runways[1]->rMutex.lock();
        runways[2]->rMutex.lock();

        if (runways[1]->useRunway())
        {
            toLandOn = runways[1];
        }
        else if (runways[2]->useRunway())
        {
            toLandOn = runways[2];
        }

        runways[1]->rMutex.unlock();
        runways[2]->rMutex.unlock();

        if (toLand && toLandOn)
        {
            coutMut.lock();
            cout << ">> Runway " << toLandOn->name << " being used for Flight " << toLand->id << endl;
            coutMut.unlock();
            // Start the flight Status Tracking Thread
            pthread_t t;
            pthread_create(&t, NULL, flightTrack, new FlightWrapper(toLand, toLandOn));
            pthread_detach(t);
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

void parse_flights_CSV(vector<Flight *> &flights, vector<Airline> &airlines, string filename)
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
            Flight *newF = new Flight(row[0], row[1], row[2], stof(row[3]), stoi(row[4]), stoi(row[5]));
            flights.push_back(newF);
        }
    }
}

void display_airlines(vector<Airline> vec)
{
    int space[] = {25, 15, 19, 20};
    coutMut.lock();
    cout << "\n\033[1mName";
    spaces("Name", space[0]);
    cout << "Type";
    spaces("Type", space[1]);
    cout << "No. of Aircrafts";
    spaces("No. of Aircrafts", space[2]);
    cout << "No. of Flights\n\033[0m";
    coutMut.unlock();

    for (int i = 0; i < vec.size(); ++i)
    {
        coutMut.lock();
        cout << vec[i].name;
        spaces(vec[i].name, space[0]);
        cout << vec[i].type;
        spaces(vec[i].type, space[1]);
        cout << vec[i].num_aircrafts;
        spaces(to_string(vec[i].num_aircrafts), space[2]);
        cout << vec[i].num_flights << endl;
        coutMut.unlock();
    }
    coutMut.lock();
    cout << endl;
    coutMut.unlock();
}

void display_flights(vector<Flight *> vec)
{
    int space[] = {12, 12, 12, 16, 12, 12, 15};
    string str;

    coutMut.lock();
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
    str = "AVN Status";
    cout << str;
    spaces(str, space[5]);
    str = "Type";
    cout << str;
    spaces(str, space[6]);
    cout << "Priority\n\033[0m";

    coutMut.unlock();

    for (int i = 0; i < vec.size(); ++i)
    {
        coutMut.lock();
        cout << vec[i]->id;
        spaces(to_string(vec[i]->id), space[0]);
        cout << vec[i]->direction;
        spaces(vec[i]->direction, space[1]);
        cout << vec[i]->status;
        spaces(vec[i]->status, space[2]);
        cout << vec[i]->phase;
        spaces(vec[i]->phase, space[3]);
        cout << to_string(vec[i]->speed);
        spaces(to_string(vec[i]->speed), space[4], 9);
        cout << vec[i]->AVN_status;
        spaces(to_string(vec[i]->AVN_status), space[5]);
        cout << vec[i]->aircraft->type;
        spaces(vec[i]->aircraft->type, space[6]);
        cout << vec[i]->priority << endl;
        coutMut.unlock();
    }
    coutMut.lock();
    cout << endl;
    coutMut.unlock();
}

void initialize_flights(string filename)
{
    parse_flights_CSV(flights_arr, airlines_arr, filename);

    int ind = 0;
    for (int i = 0; i < airlines_arr.size(); ++i)
    {
        for (int j = 0; j < airlines_vec[i].aircrafts.size(); ++j, ++ind)
        {
            flights_arr[ind]->aircraft = &airlines_arr[i].aircrafts[j];
            if (flights_arr[ind]->aircraft->type == aircraft_types[0])
                flights_arr[ind]->priority = 1;
            else if (flights_arr[ind]->aircraft->type == aircraft_types[1])
                flights_arr[ind]->priority = 2;
            else
                flights_arr[ind]->priority = 3;
            flights_arr[ind]->aircraft->speed = flights_arr[ind]->speed;
        }
    }
}

int main()
{
    //////////////////// Input ////////////////////
    string filename = "airlines_data.csv";
    parse_airlines_CSV(airlines_vec, filename);

    filename = "flights_data.csv";
    initialize_flights(filename);

    // Display data
    display_airlines(airlines_arr);
    display_flights(flights_arr);

    // Starts the Simulation, initializes variables
    StartSimulation();

    while (!flights_arr.empty() || !arrQ.empty() || !depQ.empty())
    {
        continue;
    }

    sleep(26); // To let any remaining flight finish

    simRunning = false;
    coutMut.lock();
    cout << ">> Simulation is ending." << endl;
    coutMut.unlock();
    sleep(2);

    for (Runway *x : runways)
        delete x;

    for (Flight *x : flights_arr)
        delete x;

    for (Flight *x : arrQ)
        delete x;
    for (Flight *x : depQ)
        delete x;

    return 0;
}