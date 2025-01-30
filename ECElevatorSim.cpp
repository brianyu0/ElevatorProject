#include "ECElevatorSim.h"

using namespace std;

bool ECElevatorState::CurrReq(const ECElevatorSimRequest& request, int currentTime) {
  return !request.IsServiced() && request.GetTime() <= currentTime;
}

// *************** ECElevatorStateStop CLASSES ****************
// ************************************************************
void ECElevatorStateStop::Redirect(ECElevatorSim &elevator) {
  std::vector<ECElevatorSimRequest>& requests = elevator.GetListRequests();
  int currFloor = elevator.GetCurrFloor();
  int currTime = elevator.GetCurrentTime();

  bool foundRequest = false;

  for(auto& request : requests) {
    // checks to see if any requests are from where the elevator is parked; NO LOAD TIME
    if (CurrReq(request, currTime)) {
      if (request.GetFloorSrc() == currFloor && !request.IsFloorRequestDone()) {
        // passenger is at current floor and hasn't boarded yet
        request.SetFloorRequestDone(true);
        std::cout << "Passenger boarded at floor " << currFloor << " at time " << currTime << std::endl;
        elevator.SetCurrInElevator(1);
        elevator.SetState(new ECElevatorStopOver());
        return;
      }
      if (request.GetFloorDest() == currFloor && request.IsFloorRequestDone() && !request.IsServiced()) {
        // passenger is at destination floor
        request.SetServiced(true);
        request.SetArriveTime(currTime);
        std::cout << "Passenger arrived at floor " << currFloor << " at time " << currTime << std::endl;
        elevator.SetCurrInElevator(-1);
        elevator.SetState(new ECElevatorStopOver());
        return;
      }
      foundRequest = true;
    }
  }
  if(foundRequest) {
    // move elevator;
    return;
  }
  else {
    elevator.SetCurrDir(EC_ELEVATOR_STOPPED);
  }
}
void ECElevatorStateStop::Move(ECElevatorSim &elevator) {
  std::vector<ECElevatorSimRequest>& requests = elevator.GetListRequests();
  int currFloor = elevator.GetCurrFloor();
  int nearestDistance = elevator.GetNumFloors() + 1;
  EC_ELEVATOR_DIR newDirection = EC_ELEVATOR_STOPPED;

  for (auto &request : requests) {
    if (CurrReq(request, elevator.GetCurrentTime())) {
      int distance = abs(request.GetRequestedFloor() - currFloor);
      if (distance < nearestDistance) {
        nearestDistance = distance;
        newDirection = (request.GetRequestedFloor() > currFloor) ? EC_ELEVATOR_UP : EC_ELEVATOR_DOWN; // if requested floor is > than curr, go up, else, go down
      } 
      else if (distance == nearestDistance && newDirection == EC_ELEVATOR_STOPPED) {
        newDirection = EC_ELEVATOR_UP; // if distance is the same, always go UP
      }
    }
  }
  if (newDirection != EC_ELEVATOR_STOPPED) {
    elevator.SetCurrDir(newDirection);
    elevator.SetState(new ECElevatorStateMoving());
  }
}


// *************** ECElevatorStateMoving CLASSES **************
// ************************************************************

// ********************* helper functions *********************
bool ECElevatorStateMoving::PassOff(const ECElevatorSimRequest& request, int currFloor) {
  return !request.IsServiced() && request.IsFloorRequestDone() && request.GetFloorDest() == currFloor;
}
bool ECElevatorStateMoving::PassOn(const ECElevatorSimRequest& request, int currFloor, int currTime) {
  return !request.IsFloorRequestDone() && request.GetTime() <= currTime && request.GetFloorSrc() == currFloor;
}
bool ECElevatorStateMoving::ReqCurrDirection(ECElevatorSim &elevator) {
  std::vector<ECElevatorSimRequest>& requests = elevator.GetListRequests();
  int currFloor = elevator.GetCurrFloor();
  EC_ELEVATOR_DIR currDir = elevator.GetCurrDir();
  for (auto &request : requests) {
    if (CurrReq(request, elevator.GetCurrentTime())) {
      int requestedFloor = request.IsFloorRequestDone() ? request.GetFloorDest() : request.GetFloorSrc();
      if ((currDir == EC_ELEVATOR_UP && requestedFloor > currFloor) || (currDir == EC_ELEVATOR_DOWN && requestedFloor < currFloor) || (requestedFloor == currFloor)) {
        return true;
      }
    }
  }
  return false;
}
bool ECElevatorStateMoving::NearestReq(ECElevatorSim &elevator, EC_ELEVATOR_DIR &newDir) {
  std::vector<ECElevatorSimRequest>& requests = elevator.GetListRequests();
  int currFloor = elevator.GetCurrFloor();
  int nearestDistance = elevator.GetNumFloors() + 1;
  bool hasPendingRequests = false;

  for (auto &request : requests) {
    if (CurrReq(request, elevator.GetCurrentTime())) {
      int requestedFloor = request.IsFloorRequestDone() ? request.GetFloorDest() : request.GetFloorSrc();
      int distance = abs(requestedFloor - currFloor);

      if (distance < nearestDistance) {
        nearestDistance = distance;
        newDir = (requestedFloor > currFloor) ? EC_ELEVATOR_UP : EC_ELEVATOR_DOWN;
        hasPendingRequests = true;
      }
    }
  }
  return hasPendingRequests;
}

// ******************** main two functions ********************
void ECElevatorStateMoving::Redirect(ECElevatorSim &elevator) { 
  std::vector<ECElevatorSimRequest>& requests = elevator.GetListRequests();
  int currFloor = elevator.GetCurrFloor();
  int currTime = elevator.GetCurrentTime();
  for (auto &request : requests) {
    // passengers getting OFF
    if (PassOff(request, currFloor)) {
      elevator.SetState(new ECElevatorStopOver());
      return;
    }
    // passengers getting ON
    if (PassOn(request, currFloor, currTime)) {
      request.SetFloorRequestDone(true);
      std::cout << "Passenger boarded at floor " << currFloor << " at time " << elevator.GetCurrentTime() << std::endl;
      elevator.SetCurrInElevator(1);
      elevator.SetState(new ECElevatorStopOver());
      return;
    }
  }
}
void ECElevatorStateMoving::Move(ECElevatorSim &elevator) {
  if (elevator.GetCurrentState() != this) {
    // State has changed, do not update direction
    return;
  }
  if (!ReqCurrDirection(elevator)) {
    EC_ELEVATOR_DIR newDir;
    if (NearestReq(elevator, newDir)) {
      elevator.SetCurrDir(newDir);
    } else {
      elevator.SetCurrDir(EC_ELEVATOR_STOPPED);
      elevator.SetState(new ECElevatorStateStop());
      std::cout << "Elevator has stopped due to no pending requests." << std::endl;
    }
  }
}


// **************** ECElevatorStopOver CLASSES ****************
// ************************************************************

// ********************* helper functions *********************
bool ECElevatorStopOver::GoUp(int distance, int nearestDistance, EC_ELEVATOR_DIR currDir) {
  return distance == nearestDistance && currDir != EC_ELEVATOR_DOWN;
}
bool ECElevatorStopOver::GoDown(int dest, int floor, EC_ELEVATOR_DIR dir) {
  return dest < floor && dir == EC_ELEVATOR_DOWN;
}
bool ECElevatorStopOver::NewDist(int dist, int nearestDist) {
  return dist < nearestDist;
}
EC_ELEVATOR_DIR ECElevatorStopOver::ternary(int requested, int floor, EC_ELEVATOR_DIR dir) {
  return (requested > floor || dir == EC_ELEVATOR_UP) ? EC_ELEVATOR_UP : EC_ELEVATOR_DOWN;
}

// ******************** main two functions ********************
void ECElevatorStopOver::Redirect(ECElevatorSim &elevator) {
  if (loadTime > 0) {
    // do not change direction yet; loading/unloading is not complete
    return;
  }
  // after loading/unloading, check if we need to change direction
  std::vector<ECElevatorSimRequest>& requests = elevator.GetListRequests();
  int currFloor = elevator.GetCurrFloor();
  EC_ELEVATOR_DIR newDirection = EC_ELEVATOR_STOPPED;
  int nearestDistance = elevator.GetNumFloors() + 1;

  for (auto &request : requests) {
    if (CurrReq(request, elevator.GetCurrentTime())) {
      int requestedFloor = request.IsFloorRequestDone() ? request.GetFloorDest() : request.GetFloorSrc();
      if (GoDown(request.GetFloorDest(), elevator.GetCurrFloor(), elevator.GetCurrDir())) { // if elevator already going down and has a tie, then keep going down
        newDirection = EC_ELEVATOR_DOWN;
        break;
      }
      int distance = abs(request.GetFloorDest() - currFloor);
      if (NewDist(distance, nearestDistance)) {
        nearestDistance = distance;
        newDirection = ternary(requestedFloor, currFloor, elevator.GetCurrDir());
      }
      else if (GoUp(distance, nearestDistance, elevator.GetCurrDir())) {
        newDirection = EC_ELEVATOR_UP; // if distance is the same, always go UP
      }
    }
  }
  if (newDirection != EC_ELEVATOR_STOPPED) {
    elevator.SetCurrDir(newDirection);
    elevator.SetState(new ECElevatorStateMoving());
  }
  else {
    elevator.SetCurrDir(EC_ELEVATOR_STOPPED);
    elevator.SetState(new ECElevatorStateStop());
  }
}
void ECElevatorStopOver::Move(ECElevatorSim &elevator) {
  std::vector<ECElevatorSimRequest>& requests = elevator.GetListRequests();
  int currFloor = elevator.GetCurrFloor();

  EC_ELEVATOR_DIR newDirection = EC_ELEVATOR_STOPPED;
  //Passengers leave elevator
  if(loadTime == 1){
    for (auto &request : requests) {
      if (!request.IsServiced() && request.IsFloorRequestDone() && request.GetFloorDest() == currFloor) {
        request.SetServiced(true);
        request.SetArriveTime(elevator.GetCurrentTime());
        std::cout << "Passenger arrived at floor " << currFloor << " at time " << elevator.GetCurrentTime() << std::endl;
        elevator.SetCurrInElevator(-1);
      }
    }
    // Passengers board elevator
    for (auto &request : requests) {
      if (!request.IsFloorRequestDone() && request.GetTime() <= elevator.GetCurrentTime() && request.GetFloorSrc() == currFloor) {
        // Passengers can board regardless of direction
        request.SetFloorRequestDone(true);
        std::cout << "Passenger boarded at floor " << currFloor << " at time " << elevator.GetCurrentTime() << std::endl;
        elevator.SetCurrInElevator(1);
      }
    }
  }
  if(loadTime > 0) {
    loadTime--;
  }
}



void ECElevatorMaintenance::Redirect(ECElevatorSim &elevator) {}
void ECElevatorMaintenance::Move(ECElevatorSim &elevator) {}

void ECElevatorStateStop::moveElevator(ECElevatorSim &elevator) {}
void ECElevatorStateMoving::moveElevator(ECElevatorSim &elevator) {
  int currFloor = elevator.GetCurrFloor();
  EC_ELEVATOR_DIR currDir = elevator.GetCurrDir();
  int numFloors = elevator.GetNumFloors();

  if (currDir == EC_ELEVATOR_UP && currFloor < numFloors) {
    elevator.SetCurrFloor(currFloor + 1);
  } else if (currDir == EC_ELEVATOR_DOWN && currFloor > 1) {
    elevator.SetCurrFloor(currFloor - 1);
  }
}
void ECElevatorStopOver::moveElevator(ECElevatorSim &elevator) {}
void ECElevatorMaintenance::moveElevator(ECElevatorSim &elevator) {}



// ******************* ECElevatorSim CLASSES ******************* 
// *************************************************************
ECElevatorSim :: ECElevatorSim(int numFloors, std::vector<ECElevatorSimRequest> &listRequests) : numFloors(numFloors), listRequests(listRequests), currFloor(1), currDir(EC_ELEVATOR_STOPPED), currTime(0) {
  currentState = new ECElevatorStateStop();
}
ECElevatorSim :: ~ECElevatorSim() {
  delete currentState;
}

void ECElevatorSim::Simulate(int lenSim) {
  while (currTime < lenSim) {
    std::cout << "Time: " << currTime << ", Floor: " << GetCurrFloor() << ", Dir: " << GetCurrDir() << std::endl;
    // Process new requests at currentTime
    for (auto &request : listRequests) {
      if (request.GetTime() == currTime) {
        // Request is made at this time
        std::cout << "New request: From floor " << request.GetFloorSrc() << " to floor " << request.GetFloorDest() << std::endl;
      }
    }
    currentState->Redirect(*this);
    currentState->Move(*this);
    currentState->moveElevator(*this);

    ++currTime;
  }
}

// **** new added shit
void ECElevatorSim::AdvanceOneTick() {
    // Similar to what happens for one iteration in Simulate()
    std::cout << "Time: " << currTime << ", Floor: " << GetCurrFloor() << ", Dir: " << GetCurrDir() << std::endl;
    for (auto &request : listRequests) {
        if (request.GetTime() == currTime) {
            std::cout << "New request: From floor " << request.GetFloorSrc() << " to floor " << request.GetFloorDest() << std::endl;
        }
    }

    // Let the current state handle redirection and movement
    currentState->Redirect(*this);
    currentState->Move(*this);
    currentState->moveElevator(*this);

    ++currTime;
}
// **** END

int ECElevatorSim::GetNumFloors() const {
  return numFloors;
}
int ECElevatorSim::GetCurrFloor() const {
  return currFloor;
}
void ECElevatorSim::SetCurrFloor(int f) {
  currFloor = f; // could check if f is within the allowable range
}
EC_ELEVATOR_DIR ECElevatorSim::GetCurrDir() const {
  return currDir;
}
void ECElevatorSim::SetCurrDir(EC_ELEVATOR_DIR dir) {
  currDir = dir;
}
std::vector<ECElevatorSimRequest>& ECElevatorSim::GetListRequests() {
  return listRequests;
}
ECElevatorState* ECElevatorSim::GetCurrentState() {
  return currentState;
}
void ECElevatorSim::SetState(ECElevatorState *newState) {
  if(currentState != nullptr){
    delete currentState;
  }
  currentState = newState;
  std::cout << "State changed to: " << typeid(*currentState).name() << std::endl;
}
int ECElevatorSim::GetCurrentTime() const { 
  return currTime; 
}
void ECElevatorSim::SetCurrentTime(int t) { 
  currTime = t; 
}
int ECElevatorSim::GetCurrInElevator() const {
  return currInElevator;
}
void ECElevatorSim::SetCurrInElevator(int x) {
  currInElevator += x;
}