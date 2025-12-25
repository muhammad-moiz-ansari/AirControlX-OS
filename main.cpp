/*
    Name 1: Muhammad Moiz Ansari
    Roll# 1: 23i-0523
    Name 2: Faizan-ur-Rehman Khan
    Roll# 2: 23i-3021

    Section: BS(CS)-F
    Instructor: Ms. Rabail Zahid
    OS Project - Module 3
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
#include <ctime>
#include <mutex>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using namespace std;
using namespace sf;

// Initializing Dimensions.
// resolutionX and resolutionY determine the rendering resolution.
// Don't edit unless required. Use functions on lines 43, 44, 45 for resizing the game window.
const int resolutionX = 960;
const int resolutionY = 960;
const int boxPixelsX = 32;
const int boxPixelsY = 32;
const int gameRows = resolutionX / boxPixelsX;    // Total rows on grid = 30
const int gameColumns = resolutionY / boxPixelsY; // Total columns on grid = 30

// 1 gamerow = 32 resolutions
// nth gamerow = position / boxPixels

// n gamerows = (32 * n) resolutions
// n resloutions = (n * 1.0 / 32) rows

// Initializing GameGrid.
int gameGrid[gameRows][gameColumns] = {};

// The following exist purely for readability.
const int x = 0;
const int y = 1;
const int exists = 2;

int defx = 2010,
    defy1 = 544, defy2 = 724, defy3 = 904,
    standx = 1835, liftx = 500, landx = 1200;
bool OpenNewWindow = false;

Text small_text,
    boardtext1 = small_text,
    boardtext2 = small_text,
    boardtext3 = small_text,
    violationstext1 = small_text,
    violationstext2 = small_text;

string airline_types[] = {"Commercial", "Military", "Medical", "Cargo"},
       aircraft_types[] = {"Commercial", "Cargo", "Emergency"},
       flight_status[] = {"Arrival", "Departure", "Emergency"},
       arrival_phases[] = {"Holding Phase", "Approach Phase", "Landing Phase", "Taxi Phase", "At Gate"},
       departure_phases[] = {"At Gate", "Taxi Phase", "Takeoff Roll", "Climb", "Departure"},
       rname[] = {"RWY-A", "RWY-B", "RWY-C"};

int flight_id_assign = 1; // Variable for assigning flight ids
int avn_id_assign = 1;    // Variable for assigning avn ids
int no_of_airlines = 6;
int no_of_activeViolations = 0;
mutex coutMut;

string getDateTime(int daysAdd = 0);
void spaces(string str, int s, int limit = 99)
{
    int size = s - str.size();
    if (size > limit)
        size = limit;
    for (int i = 0; i < size; ++i)
        cout << " ";
}

// Pipe from flight tracking ---> AVN process
int flight_to_avn[2];
int atcs_to_avn[2], avn_to_atcs[2],
    avn_to_portal[2],
    avn_to_stripepay[2], stripepay_to_avn[2],
    avn_to_sfml[2];

/////////////////////////////
//                         //
//          PLANE          //
//                         //
/////////////////////////////

class Plane
{
public:
    float x, y;
    float speed, spixel;
    string color;
    Plane(float X = defx, float Y = defy1, float sp = 0, float spix = 0)
    {
        x = X;
        y = Y;
        speed = sp;
        spixel = spix;
    }
};

Plane plane1(defx, defy1), plane2(defx, defy2), plane3(defx, defy3);
sf::Sprite planeSprite1, planeSprite2, planeSprite3;

void set_speed(string name, float speed, float fact)
{
    if (name == rname[0])
        plane1.spixel = speed * fact;
    else if (name == rname[1])
        plane2.spixel = speed * fact;
    else // if (name == rname[2])
        plane3.spixel = speed * fact;
}

void set_position(string name) // Sets the plane position to defualy position based on runway's name
{
    if (name == rname[0])
        plane1.x = defx;
    else if (name == rname[1])
        plane2.x = defx;
    else // if (name == rname[2])
        plane3.x = defx;
}

void set_text(string name, string text)
{
    if (name == rname[0])
        boardtext1.setString(text);
    else if (name == rname[1])
        boardtext2.setString(text);
    else // if (name == rname[2])
        boardtext3.setString(text);
}

void set_color(string type)
{
    if (type == aircraft_types[0])
        planeSprite1.setColor(sf::Color(220, 255, 220)); // White (Green)
    else if (type == aircraft_types[1])
        planeSprite2.setColor(sf::Color(173, 216, 240)); // Blue
    else                                                 // if (type == aircraft_types[2])
        planeSprite3.setColor(sf::Color(255, 128, 128)); // Red
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
    int total_violations;

    Airline(string n = "", string t = "", int no_air = 0, int f = 0)
    {
        name = n;
        type = t;
        num_aircrafts = no_air;
        num_flights = f;

        total_violations = 0;

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
    float speed, altitude, posx, posy;
    bool AVN_status;
    Aircraft *aircraft;

    Flight(string d, string st = "", string ph = "", float sp = 0, float alt = 0, float x = defx, float y = defy1, int pr = 0, bool avns = 0)
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
        speed = sp;
        altitude = alt;
        posx = x;
        posy = y;
        priority = pr; // Actual priority is assigned later according to aircraft
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
    int flight_id;

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

////////////////////////////////////////////
//                                        //
//                  AVN                   //
//                                        //
////////////////////////////////////////////
class AVN
{
public:
    int avn_id, flight_number;
    string airline_name, aircraft_type;
    float recorded_speed, allowed_speed_min, allowed_speed_max;
    int altitude, allowed_altitude_min, allowed_altitude_max;
    string issue_date_time, due_date;
    float fine_amount;
    string payment_status;

    AVN(int fltId = 0, string alineName = "", string acraftType = "", float recSpeed = 0, float allowSpeedMin = 0, float allowSpeedMax = 0, int alt = 0, int allowAltMin = 0, int allowAltMax = 0, string issue_DT = "", string due = "", float amount = 0, string payStatus = "Unpaid")
    {
        avn_id = avn_id_assign++;
        flight_number = fltId;
        airline_name = alineName;
        aircraft_type = acraftType;
        recorded_speed = recSpeed;
        allowed_speed_min = allowSpeedMin;
        allowed_speed_max = allowSpeedMax;
        altitude = alt;
        allowed_altitude_min = allowAltMin;
        allowed_altitude_max = allowAltMax;
        issue_date_time = issue_DT;
        due_date = due;
        fine_amount = amount;
        payment_status = payStatus;
    }
};
vector<AVN> avn_list;
mutex avnMutex;

///////////////////////////////////////////////
//                                           //
//              FLIGHT WRAPPER               //
//                                           //
///////////////////////////////////////////////

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

/////////////////////////////
//                         //
//       FLIGHT DATA       //
//                         //
/////////////////////////////

// Class for storing flight data and transfer it for communication
class FlightData
{
public:
    Flight *flt_ptr;
    int flightId,
        posx, posy;
    int speed, allowed_speed_min, allowed_speed_max;
    int altitude, allowed_altitude_min, allowed_altitude_max;
    char phase[50], aircraft_type[50], airline_name[50], runway_name[50];
    bool speed_violation, altitude_violation, position_violation;

    FlightData(Flight *ptr = NULL, int ID = 0, float sp = 0, float allow_sp_min = 0, float allow_sp_max = 0, float alt = 0, float allow_alt_min = 0, float allow_alt_max = 0, string ph = "", string acraft_type = "", string aline_name = "", int POSX = 0, int POSY = 0)
    {
        flt_ptr = ptr;
        flightId = ID;
        speed = sp;
        allowed_speed_min = allow_sp_min;
        allowed_speed_max = allow_sp_max;
        altitude = alt;
        allowed_altitude_min = allow_alt_min;
        allowed_altitude_max = allow_alt_max;
        strcpy(phase, ph.c_str());
        strcpy(aircraft_type, acraft_type.c_str());
        strcpy(airline_name, aline_name.c_str());
        speed_violation = false;
        altitude_violation = false;
        position_violation = false;
        posx = POSX;
        posy = POSY;
        speed_violation = false;
        altitude_violation = false;
        position_violation = false;
    }
};

void *flightTrack(void *arg)
{
    close(atcs_to_avn[0]);

    FlightWrapper *fw = (FlightWrapper *)arg;

    Flight *flt = fw->toLand;
    Runway *rw = fw->toLandOn;

    string str, airline_name;

    for (int i = 0; i < airlines_arr.size(); ++i)
    {
        for (int j = 0; j < airlines_arr[i].num_aircrafts; ++j)
        {
            if (flt->aircraft->id == airlines_arr[i].aircrafts[j].id)
                airline_name = airlines_arr[i].name;
        }
    }
    FlightData data;
    strcpy(data.airline_name, airline_name.c_str());

    // Arrival Handling
    if (flt->status == flight_status[0])
    {
        // FlightData data;
        // strcpy(data.airline_name, airline_name.c_str());
        //  Holding Phase
        sleep(5);
        flt->phase = arrival_phases[0];
        flt->speed = 400 + (rand() % 705);
        flt->altitude = 10000 + (rand() % 1100 - 700);
        flt->posx = 2000 + (rand() % 500);
        flt->posy = 500 + (rand() % 500);
        data.allowed_speed_min = 400;
        data.allowed_speed_max = 600;
        data.allowed_altitude_min = 9500;
        data.allowed_altitude_max = 12000;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (flt->altitude < data.allowed_altitude_min)
            data.altitude_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[0] << endl;
        coutMut.unlock();
        set_speed(rw->name, flt->speed, 0.00);
        str = "Flight " + to_string(flt->id) + " -> " + arrival_phases[0];
        set_text(rw->name, str);

        // Approach Phase
        sleep(5);
        flt->phase = arrival_phases[1];
        flt->speed = 240 + (rand() % 55);
        flt->altitude = 5000 + (rand() % 500 - 250);
        flt->posx = 1500;
        if (rw->name == rname[0])
            flt->posy = defy1;
        else if (rw->name == rname[1])
            flt->posy = defy2;
        else
            flt->posy = defy3;
        data.allowed_speed_min = 240;
        data.allowed_speed_max = 290;
        data.allowed_altitude_min = 4750;
        data.allowed_altitude_max = 5250;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (flt->altitude < data.allowed_altitude_min || flt->altitude > data.allowed_altitude_max)
            data.altitude_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[1] << endl;
        coutMut.unlock();
        set_speed(rw->name, flt->speed, 0.03);
        str = "Flight " + to_string(flt->id) + " -> " + arrival_phases[1];
        set_text(rw->name, str);

        // Landing Phase
        sleep(5);
        flt->phase = arrival_phases[2];
        flt->speed = 30 + (rand() % 215);
        flt->altitude = 1000 + (rand() % 200 - 100);
        flt->posx = 1200;
        if (rw->name == rname[0])
            flt->posy = defy1;
        else if (rw->name == rname[1])
            flt->posy = defy2;
        else
            flt->posy = defy3;
        data.allowed_speed_min = 30;
        data.allowed_speed_max = 240;
        data.allowed_altitude_min = 900;
        data.allowed_altitude_max = 1100;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (flt->altitude < data.allowed_altitude_min || flt->altitude > data.allowed_altitude_max)
            data.altitude_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[2] << endl;
        coutMut.unlock();
        set_speed(rw->name, flt->speed, 0.06);
        str = "Flight " + to_string(flt->id) + " -> " + arrival_phases[2];
        set_text(rw->name, str);

        // Taxi Phase
        sleep(5);
        flt->phase = arrival_phases[3];
        flt->speed = 15 + (rand() % 20);
        flt->altitude = 0;
        flt->posx = 1000;
        if (rw->name == rname[0])
            flt->posy = defy1;
        else if (rw->name == rname[1])
            flt->posy = defy2;
        else
            flt->posy = defy3;
        data.allowed_speed_min = 15;
        data.allowed_speed_max = 30;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[3] << endl;
        coutMut.unlock();
        set_speed(rw->name, flt->speed, 0.08);
        str = "Flight " + to_string(flt->id) + " -> " + arrival_phases[3];
        set_text(rw->name, str);

        // At Gate
        sleep(5);
        flt->phase = arrival_phases[4];
        flt->speed = 0 + (rand() % 20);
        flt->altitude = 0;
        flt->posx = 800;
        if (rw->name == rname[0])
            flt->posy = defy1;
        else if (rw->name == rname[1])
            flt->posy = defy2;
        else
            flt->posy = defy3;
        data.allowed_speed_min = 0;
        data.allowed_speed_max = 10;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << arrival_phases[4] << endl;
        coutMut.unlock();
        set_speed(rw->name, flt->speed, 0.08);
        str = "Flight " + to_string(flt->id) + " -> " + arrival_phases[4];
        set_text(rw->name, str);

        coutMut.lock();
        cout << ">> Flight " << flt->id << " has Landed." << endl;
        cout << ">> Runway " << rw->name << " is Freed Up." << endl;
        coutMut.unlock();
        str = "Runway " + rw->name + " is Freed Up";
        set_text(rw->name, str);

        rw->freeRunway();
    }
    // Departure Handling
    else if (flt->status == flight_status[1])
    {
        // FlightData data;
        // data.airline_name = airline_name;
        //  At Gate
        sleep(5);
        flt->phase = departure_phases[0];
        flt->speed = 0 + (rand() % 15);
        flt->altitude = 0;
        flt->posx = 800;
        if (rw->name == rname[0])
            flt->posy = defy1;
        else if (rw->name == rname[1])
            flt->posy = defy2;
        else
            flt->posy = defy3;
        data.allowed_speed_min = 0;
        data.allowed_speed_max = 10;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[0] << endl;
        coutMut.unlock();
        set_speed(rw->name, flt->speed, 0.1);
        str = "Flight " + to_string(flt->id) + " -> " + departure_phases[0];
        set_text(rw->name, str);

        // Taxi Phase
        sleep(5);
        flt->phase = departure_phases[1];
        flt->speed = 15 + (rand() % 20);
        flt->altitude = 0;
        flt->posx = 1000;
        if (rw->name == rname[0])
            flt->posy = defy1;
        else if (rw->name == rname[1])
            flt->posy = defy2;
        else
            flt->posy = defy3;
        data.allowed_speed_min = 15;
        data.allowed_speed_max = 30;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[1] << endl;
        coutMut.unlock();
        set_speed(rw->name, flt->speed, 0.2);
        str = "Flight " + to_string(flt->id) + " -> " + departure_phases[1];
        set_text(rw->name, str);

        // Takeoff Roll
        sleep(5);
        flt->phase = departure_phases[2];
        flt->speed = 0 + (rand() % 295);
        flt->altitude = 500;
        flt->posx = 1200;
        if (rw->name == rname[0])
            flt->posy = defy1;
        else if (rw->name == rname[1])
            flt->posy = defy2;
        else
            flt->posy = defy3;
        data.allowed_speed_min = 30;
        data.allowed_speed_max = 290;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[2] << endl;
        coutMut.unlock();
        set_speed(rw->name, flt->speed, 0.1);
        str = "Flight " + to_string(flt->id) + " -> " + departure_phases[2];
        set_text(rw->name, str);

        // Climb
        sleep(5);
        flt->phase = departure_phases[3];
        flt->speed = 250 + (rand() % 215);
        flt->altitude = 5000 + (rand() % 500 - 250);
        flt->posx = 1500;
        flt->posy = 500 + (rand() % 500);
        data.allowed_speed_min = 250;
        data.allowed_speed_max = 463;
        data.allowed_altitude_min = 5000;
        data.allowed_altitude_max = 11000;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (flt->altitude < data.allowed_altitude_min || flt->altitude > data.allowed_altitude_max)
            data.altitude_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[3] << endl;
        coutMut.unlock();
        set_speed(rw->name, flt->speed, 0.1);
        str = "Flight " + to_string(flt->id) + " -> " + departure_phases[3];
        set_text(rw->name, str);

        // Departure
        sleep(5);
        flt->phase = departure_phases[4];
        flt->speed = 800 + (rand() % 105);
        flt->altitude = 12000 + (rand() % 1000 - 500);
        flt->posx = 2000 + (rand() % 500);
        flt->posy = 500 + (rand() % 500);
        data.allowed_speed_min = 800;
        data.allowed_speed_max = 900;
        data.allowed_altitude_min = 10000;
        data.allowed_altitude_max = 12200;
        data.speed_violation = false;
        data.altitude_violation = false;

        if (flt->speed < data.allowed_speed_min || flt->speed > data.allowed_speed_max)
            data.speed_violation = 1;
        if (flt->altitude < data.allowed_altitude_min || flt->altitude > data.allowed_altitude_max)
            data.altitude_violation = 1;
        if (data.speed_violation || data.altitude_violation || data.position_violation)
        {
            coutMut.lock();
            cout << "   Violation Detected!\n";
            coutMut.unlock();
            strcpy(data.aircraft_type, flt->aircraft->type.c_str());
            data.altitude = flt->altitude;
            data.flightId = flt->id;
            strcpy(data.runway_name, rw->name.c_str());
            strcpy(data.phase, flt->phase.c_str());
            data.speed = flt->speed;
            data.posx = flt->posx;
            data.posy = flt->posy;

            write(atcs_to_avn[1], &data, sizeof(FlightData));
        }
        coutMut.lock();
        cout << ">> Flight " << flt->id << " -> " << departure_phases[4] << endl;
        coutMut.unlock();
        str = "Flight " + to_string(flt->id) + " -> " + departure_phases[4];
        set_text(rw->name, str);
        set_speed(rw->name, flt->speed, 0.1);

        coutMut.lock();
        cout << ">> Flight " << flt->id << " has Departed." << endl;
        cout << ">> Runway " << rw->name << " is Freed Up." << endl;
        coutMut.unlock();
        str = "Runway " + rw->name + " is Freed Up";
        set_text(rw->name, str);

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
            string str = "Runway being used for Flight " + to_string(toLand->id);
            set_text(toLandOn->name, str);
            set_color(toLand->aircraft->type);
            set_position(toLandOn->name);
            toLandOn->flight_id = toLand->id;

            //  Start the flight Status Tracking Thread
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
            string str = "Runway being used for Flight " + to_string(toLand->id);
            set_text(toLandOn->name, str);
            set_color(toLand->aircraft->type);
            set_position(toLandOn->name);
            toLandOn->flight_id = toLand->id;

            //  Start the flight Status Tracking Thread
            pthread_t t;
            pthread_create(&t, NULL, flightTrack, new FlightWrapper(toLand, toLandOn));
            pthread_detach(t);
        }
    }
    pthread_exit(NULL);
}

void avnGeneratorProcess()
{
    close(atcs_to_avn[1]);
    close(avn_to_atcs[0]);
    //// close(avn_to_atcs[1]);
    // close(avn_to_portal[0]);
    // close(avn_to_stripepay[0]);
    // close(stripepay_to_avn[1]);
    //close(avn_to_sfml[0]);

    while (true)
    {
        // coutMut.lock();
        // cout << "\nAVN Running\n";
        // coutMut.unlock();
        ++no_of_activeViolations;
        float fine = 0;
        string issueDateTime = getDateTime(), dueDate = getDateTime(3);
        FlightData data;
        int bytesread = read(atcs_to_avn[0], &data, sizeof(data));
        cout << "Data Read\n";
        char atcmsg[256];
        string atcstr;

        if (bytesread <= 0)
        {
            cout << "No flight data read\n";
            continue;
        }

        else
        {
            coutMut.lock();
            cout << "\nAVN recieved data!\n";
            if (data.speed_violation)
                atcstr = "-- Flight " + to_string(data.flightId) + " caught on violation at speed " + to_string(data.speed) + " km/h with speed limit (" + to_string(data.allowed_speed_min) + "-" + to_string(data.allowed_speed_max) + ")";
            if (data.altitude_violation)
                atcstr = "-- Flight " + to_string(data.flightId) + " caught on violation at altitude " + to_string(data.altitude) + " ft with height limit (" + to_string(data.allowed_altitude_min) + "-" + to_string(data.allowed_altitude_max) + ")";
            cout << atcstr << endl;
            coutMut.unlock();

            //write(avn_to_sfml[1], atcstr.c_str(), atcstr.length());
        }
        //        strcpy(atcmsg, atcstr.c_str());
        //        write(avn_to_atcs[1], atcstr.c_str(), atcstr.length());
        //        // Check if data is available to read
        //        fd_set readfds;
        //        FD_ZERO(&readfds);
        //        FD_SET(avn_to_atcs[0], &readfds);
        //        struct timeval timeout = {1, 0}; // 1 second timeout
        //        int result = select(avn_to_atcs[0] + 1, &readfds, NULL, NULL, &timeout);
        //        if (result > 0 && FD_ISSET(avn_to_atcs[0], &readfds))
        //            std::cout << "Data available to read from pipe!" << std::endl;
        //        else
        //            std::cout << "No data available." << std::endl;
        //        violationstext.setString(atcstr);
        //
        //        if (data.aircraft_type == "Commercial")
        //            fine = 500000;
        //        else if (data.aircraft_type == "Cargo")
        //            fine = 700000;
        //        fine *= 1.15; // 15% service charges
        //
        //        AVN avn(data.flightId, data.airline_name, data.aircraft_type, data.speed, data.allowed_speed_min, data.allowed_speed_max, data.altitude, data.allowed_altitude_min, data.allowed_altitude_max, issueDateTime, dueDate, fine);
        //        avn_list.push_back(avn);
        //        data.flt_ptr->AVN_status = true;
        //
        //        // Print messages
        //        char vmsg[300];
        //        string temp = "AVN " + to_string(avn.avn_id) + " for Flight " + to_string(avn.flight_number) + ": ";
        //        if (data.speed_violation)
        //            temp += "Speed Violation: Recorded speed = " + to_string(avn.recorded_speed) + "km/h, Allowed speed = (" + to_string(avn.allowed_speed_min) + "-" + to_string(avn.allowed_speed_max) + ") ";
        //        if (data.speed_violation && data.altitude_violation)
        //            temp += ", ";
        //        if (data.altitude_violation)
        //            temp += "Altitude Violation Recorded altitude = " + to_string(avn.altitude) + "km/h, Allowed altitude = (" + to_string(avn.allowed_altitude_min) + "-" + to_string(avn.allowed_altitude_max) + ")";
        //
        //        coutMut.lock();
        //        cout << temp << endl;
        //        coutMut.unlock();
        //        strcpy(vmsg, temp.c_str());
        //
        //        // write(avn_to_sfml[1], &vmsg, sizeof(vmsg));
        //        write(avn_to_portal[1], &avn, sizeof(AVN));
        //        write(avn_to_stripepay[1], &avn, sizeof(AVN));
        //
        //        // Reading info from stripe pay
        //        AVN new_avn;
        //        read(stripepay_to_avn[0], &new_avn, sizeof(AVN));
        //
        //        for (int i = 0; i < avn_list.size(); ++i)
        //        {
        //            if (new_avn.avn_id == avn_list[i].avn_id)
        //                avn_list[i].payment_status = new_avn.payment_status;
        //        }
        //        char msg[256];
        //        string str = "Airline \"" + new_avn.airline_name + "\" with AVN Id: " + to_string(new_avn.avn_id) + " has completed payment\n";
        //        strcpy(msg, str.c_str());
        //        write(avn_to_portal[1], msg, strlen(msg) + 1);
    }
}

void airlinePortalProcess()
{
    if (OpenNewWindow == true)
    {
    }
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

string getDateTime(int daysAdd)
{
    time_t now = time(0);          // current time
    now += daysAdd * 24 * 60 * 60; // add days in seconds

    tm *futuretime = localtime(&now);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", futuretime);

    return std::string(buffer);
}

int main()
{
    srand(time(0));
    //////////////////// SFML Setting ////////////////////
    // Declaring RenderWindow.
    int windowposx = 300, windowposy = 0;
    sf::RenderWindow window(sf::VideoMode(2000, 1105), "AirControlX - Flight Control System", sf::Style::Close | sf::Style::Titlebar);

    // Used to resize your window if it's too big or too small. Use according to your needs.
    window.setSize(sf::Vector2u(1530, 935)); // Recommended for 1366x768 (768p) displays.
    // window.setSize(sf::Vector2u(1280, 1280)); // Recommended for 2560x1440 (1440p) displays.

    // Used to position your window on every launch. Use according to your needs.
    window.setPosition(sf::Vector2i(windowposx, windowposy));

    //////////////////// Initializing Textures ////////////////////

    plane1.x = defx;
    plane1.y = defy1;
    plane2.x = defx;
    plane2.y = defy2;
    plane3.x = defx;
    plane3.y = defy3;

    // Initializing Text.
    Font font;
    font.loadFromFile("Fonts/DOS_VGA.ttf");

    Text big_text;
    big_text.setFont(font);
    big_text.setCharacterSize(55);
    big_text.setFillColor(Color::Black);

    small_text.setFont(font);
    small_text.setCharacterSize(32);
    small_text.setFillColor(Color::Black);

    int big_gap = 70, big_posx = 720, big_posy = 175,
        small_gap = 72, small_posx = 1000, small_posy = 192;
    // Plane 1
    Text plane1_text = big_text;
    plane1_text.setString("Runway A: ");
    plane1_text.setPosition(big_posx, big_posy + big_gap * 0);

    boardtext1 = small_text;
    boardtext1.setString("________________________");
    boardtext1.setPosition(small_posx, small_posy + small_gap * 0);

    // Plane 2
    Text plane2_text = big_text;
    plane2_text.setString("Runway B: ");
    plane2_text.setPosition(big_posx, big_posy + big_gap * 1);

    boardtext2 = small_text;
    boardtext2.setString("________________________");
    boardtext2.setPosition(small_posx, small_posy + small_gap * 1);

    // Plane 3
    Text plane3_text = big_text;
    plane3_text.setString("Runway C: ");
    plane3_text.setPosition(big_posx, big_posy + big_gap * 2);

    boardtext3 = small_text;
    boardtext3.setString("________________________");
    boardtext3.setPosition(small_posx, small_posy + small_gap * 2);

    violationstext1 = small_text;
    violationstext1.setString("No violations...");
    violationstext1.setPosition(big_posx - 400, small_posy - 100);

    violationstext2 = small_text;
    violationstext2.setString("...");
    violationstext2.setPosition(big_posx, small_posy - 50);

    // Initializing Background.
    sf::Texture backgroundTexture1;
    sf::Sprite backgroundSprite1;
    backgroundTexture1.loadFromFile("Textures/land.png");
    backgroundSprite1.setTexture(backgroundTexture1);
    backgroundSprite1.setColor(sf::Color(255, 255, 255)); // Reduces Opacity to 35%

    // Runway 1
    sf::Texture runwayTexture1;
    sf::Texture runwayTexture2;
    sf::Texture runwayTexture3;
    sf::Sprite runwaySprites;
    runwayTexture1.loadFromFile("Textures/road1.png");
    runwayTexture2.loadFromFile("Textures/road2.png");
    runwayTexture3.loadFromFile("Textures/road3.png");
    runwaySprites.setTexture(runwayTexture1);

    // ATC Tower
    sf::Texture atcTexture;
    sf::Sprite atcSprite;
    atcTexture.loadFromFile("Textures/atc.png");
    atcSprite.setTexture(atcTexture);
    atcSprite.setPosition(120, 40);
    atcSprite.setScale(0.27, 0.27);

    // Board Tower
    sf::Texture boardTexture;
    sf::Sprite boardSprite;
    boardTexture.loadFromFile("Textures/board.png");
    boardSprite.setTexture(boardTexture);
    boardSprite.setPosition(650, 10);
    boardSprite.setScale(2.6, 2.4);

    // Portal button
    int button_sizex = 225, button_sizey = 68,
        button_posx = 350, button_posy = 380;
    sf::Texture buttonTexture;
    sf::Sprite buttonSprite;
    buttonTexture.loadFromFile("Textures/button.png");
    buttonSprite.setTexture(buttonTexture);
    buttonSprite.setPosition(button_posx, button_posy);
    buttonSprite.setScale(0.5, 0.5);

    int runwayx = 100, runwayy = 565;
    // Plane
    sf::Texture planeTexture;
    planeTexture.loadFromFile("Textures/plane_y.png");
    planeSprite1.setTexture(planeTexture);
    planeSprite1.setScale(0.28, 0.28);
    planeSprite2 = planeSprite1;
    planeSprite3 = planeSprite1;
    planeSprite1.setPosition(plane1.x, plane1.y);
    planeSprite2.setPosition(plane2.x, plane2.y);
    planeSprite3.setPosition(plane3.x, plane3.y);
    set_color(aircraft_types[0]);
    set_color(aircraft_types[1]);
    set_color(aircraft_types[2]);

    //////////////////// Input ////////////////////
    string filename = "airlines_data.csv";
    parse_airlines_CSV(airlines_vec, filename);

    filename = "flights_data.csv";
    initialize_flights(filename);

    // Display data
    display_airlines(airlines_arr);
    display_flights(flights_arr);

    ////////////////// DECLARE PIPES //////////////////

    // int atcs_to_avn[2],
    //     avn_to_portal[2],
    //     avn_to_stripepay[2], stripepay_to_avn[2];

    // pipe(flight_to_avn);
    pipe(atcs_to_avn);
    pipe(avn_to_atcs);
    //pipe(avn_to_sfml);
    // pipe(avn_to_portal);
    // pipe(avn_to_stripepay);
    // pipe(stripepay_to_avn);
    //int flags = fcntl(avn_to_sfml[0], F_GETFL, 0);
    //fcntl(avn_to_sfml[0], F_SETFL, flags | O_NONBLOCK);

    ////////////////// FORKING PROCESSES //////////////////
    pid_t avn_pid, portal_pid, aportal_pid;
    avn_pid = fork();
    if (avn_pid == 0) // AVN Process
    {
        avnGeneratorProcess(); // atcs_to_avn, avn_to_atcs, avn_to_portal, avn_to_stripepay, stripepay_to_avn);
        exit(0);
    }
    else if (avn_pid < 0)
        cout << "Error in forking AVN!\n";
    else // Parent process
    {
        portal_pid = fork();
        if (portal_pid == 0) // Airline PORTAL Process
        {
            // close(atcs_to_avn[0]);
            close(atcs_to_avn[1]);
            close(avn_to_atcs[0]);
            close(avn_to_atcs[1]);
            close(avn_to_portal[1]);
            close(avn_to_stripepay[0]);
            close(avn_to_stripepay[1]);
            close(stripepay_to_avn[0]);
            close(stripepay_to_avn[1]);
            //close(avn_to_sfml[0]);
            //close(avn_to_sfml[1]);

            airlinePortalProcess();

            exit(0);
        }
        else if (avn_pid < 0)
            cout << "Error in forking AVN!\n";
        else // Parent process
        {

            cout << "Parent\n";

            // Closing unused pipe ends in parent
            // close(flight_to_atcs[0]);
            // close(atcs_to_avn[0]);
            // close(atcs_to_avn[1]);
            // close(avn_to_atcs[0]);
            close(avn_to_atcs[1]);
            close(avn_to_portal[0]);
            // close(avn_to_portal[1]);
            close(avn_to_stripepay[0]);
            // close(avn_to_stripepay[1]);
            // close(stripepay_to_avn[0]);
            close(stripepay_to_avn[1]);
            //close(avn_to_sfml[1]);

            bool simulationFinished = false;
            sf::Clock postSimClock; // Starts the clock
            bool startedPostSimTimer = false;

            // Starts the Simulation, initializes variables
            StartSimulation();

            //////////////////// SFML Render Loop ////////////////////
            while (window.isOpen())
            {
                sf::Event e;
                while (window.pollEvent(e))
                {
                    if (e.type == sf::Event::Closed || (e.KeyPressed && e.key.code == Keyboard::Escape))
                        window.close();
                    if (e.type == Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left && Mouse::getPosition().x >= button_posx && Mouse::getPosition().x <= (button_posx + button_sizex) && Mouse::getPosition().y >= button_posy && Mouse::getPosition().y <= (button_posy + button_sizey))
                    {
                        /*
                        portal_pid = fork();
                        if (portal_pid == 0) // Airline PORTAL Process
                        {
                            // close(atcs_to_avn[0]);
                            close(atcs_to_avn[1]);
                            close(avn_to_atcs[0]);
                            close(avn_to_atcs[1]);
                            close(avn_to_portal[1]);
                            close(avn_to_stripepay[0]);
                            close(avn_to_stripepay[1]);
                            close(stripepay_to_avn[0]);
                            close(stripepay_to_avn[1]);
                            close(avn_to_sfml[0]);
                            close(avn_to_sfml[1]);

                            sf::RenderWindow newWindow(sf::VideoMode(2000, 1105), "Airline Portal");

                            while (newWindow.isOpen())
                            {
                                sf::Event newEvent;
                                while (newWindow.pollEvent(newEvent))
                                {
                                    if (newEvent.type == sf::Event::Closed)
                                        newWindow.close();
                                }

                                newWindow.display();
                                newWindow.clear(sf::Color::White);
                            }
                            
                            

                            exit(0);
                        }
                        else if (avn_pid < 0)
                            cout << "Error in forking Portal!\n";
                        */
                    }

                    // Check simulation status
                    if (!simulationFinished && flights_arr.empty() && arrQ.empty() && depQ.empty())
                    {
                        simRunning = false;
                        coutMut.lock();
                        cout << ">> Simulation is ending. Displaying for 28 seconds..." << endl;
                        coutMut.unlock();
                        postSimClock.restart(); // Start the 26-second timer
                        startedPostSimTimer = true;
                        simulationFinished = true;
                    }

                    // If simulation ended and 26 seconds have passed
                    if (startedPostSimTimer && postSimClock.getElapsedTime().asSeconds() >= 28)
                    {
                        // window.close();
                    }

                    // --- Render scene ---

                    window.draw(backgroundSprite1);
                    window.draw(atcSprite);
                    window.draw(boardSprite);
                    window.draw(plane1_text);
                    window.draw(plane2_text);
                    window.draw(plane3_text);
                    window.draw(boardtext1);
                    window.draw(boardtext2);
                    window.draw(boardtext3);
                    window.draw(buttonSprite);

                    /*
                    char avnmsg[300];
                    int numread = read(avn_to_sfml[0], avnmsg, 300);
                    string temp1(avnmsg), temp2 = "...";
                    // temp2=temp1.substr(56, temp1.length()-56);
                    // temp1=temp1.substr(0, 56);

                    if (numread > 0)
                    {
                        violationstext1.setString(temp1);
                        //violationstext2.setString(temp2);
                    }
                    */
                    window.draw(violationstext1);
                    //window.draw(violationstext2);

                    runwayx = 95;
                    runwayy = 565;
                    int planescale = 0.5;
                    for (int i = 0; i < runways.size(); ++i)
                    {

                        if (i == 0)
                            runwaySprites.setTexture(runwayTexture1);
                        if (i == 1)
                            runwaySprites.setTexture(runwayTexture2);
                        else if (i == 2)
                            runwaySprites.setTexture(runwayTexture3);
                        runwaySprites.setScale(1.37, 1);
                        runwaySprites.setPosition(runwayx, runwayy + i * 180);
                        window.draw(runwaySprites);
                    }

                    plane1.x -= plane1.spixel;
                    plane2.x -= plane2.spixel;
                    plane3.x -= plane3.spixel;

                    planeSprite1.setPosition(plane1.x, plane1.y);
                    planeSprite2.setPosition(plane2.x, plane2.y);
                    planeSprite3.setPosition(plane3.x, plane3.y);

                    window.draw(planeSprite1);
                    window.draw(planeSprite2);
                    window.draw(planeSprite3);

                    window.display();
                    window.clear();
                }

                //////////////////// Cleanup ////////////////////
                for (Runway *x : runways)
                    delete x;

                for (Flight *x : flights_arr)
                    delete x;

                for (Flight *x : arrQ)
                    delete x;

                for (Flight *x : depQ)
                    delete x;

                // Waiting for children
                // wait(NULL);
                // wait(NULL);
                // wait(NULL);
                waitpid(avn_pid, NULL, 0);
                waitpid(portal_pid, NULL, 0);
            }

            return 0;
        }
    }
}