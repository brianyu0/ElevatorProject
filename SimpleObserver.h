#ifndef SimpleObserver_h
#define SimpleObserver_h

#include "ECObserver.h"
#include "ECGraphicViewImp.h"
#include <vector>
#include <iostream>
#include "../BACK_END/ECElevatorSim.h"

//************************************************************
class ECSimpleGraphicObserver : public ECObserver
{
public:
    ECSimpleGraphicObserver( ECGraphicViewImp &viewIn, ECElevatorSim &simIn, int totalTicks );
    virtual void Update();

    
private:
    ECGraphicViewImp &view;
    ECElevatorSim &sim;
    int cabinY;          // y-coord of cabin
    bool movingUp;     
    bool movingDown;     
    int cabinSpeed;     
    int targetY;         // y-coord of where the next nearest floor is at
    std::vector<int> floorPositions; // y-coords of the floors
    int totalFloors = 5;
    int floorHeight = 1500 / totalFloors; // Spacing between floors
    int baseY = 1750 - floorHeight;      // Starting position for floor 1

    // void DrawEverything();
    void DrawProgressBar();
    void MoveElevator();
    int GetNextFloorY(bool goingUp);
    int GetCurrentFloor();

    // For buttons
   struct Button {
    int centerX;
    int centerY;
    int width;
    int height;
    int floor;
    int direction;
    ECGVColor color;  
    };
    std::vector<Button> buttons;

    void CreateButtons();
    bool IsClickOnButton(int x, int y, const Button &button);
    
    void DrawWaitingPassengers();

    void RedrawElevatorSystem();

    int waitingPassengers[5][2]; // [floor][direction], 0=up, 1=down

    // number of passengers in cabin
    int numPassengersCabin;

    bool paused;
    int totalTicks;

    bool isMoving;
    bool waitingForFrontEnd;
};

#endif /* SimpleObserver_h */
