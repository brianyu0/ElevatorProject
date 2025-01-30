#include "SimpleObserver.h"
#include <cmath>
#include <climits>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>

//************************************************************

ECSimpleGraphicObserver::ECSimpleGraphicObserver(ECGraphicViewImp &viewIn, ECElevatorSim &simIn, int totalTicksIn) : view(viewIn), sim(simIn), totalTicks(totalTicksIn), movingUp(false), movingDown(false), cabinSpeed(5), targetY(-1), numPassengersCabin(0), paused(false), isMoving(false), waitingForFrontEnd(false)
{

  cabinY = 1450;

  // // y-coords of center of each floor
  // floorPositions[0] = 1450; // Floor 1
  // floorPositions[1] = 1150; // Floor 2
  // floorPositions[2] = 850;  // Floor 3
  // floorPositions[3] = 550;  // Floor 4
  // floorPositions[4] = 250;  // Floor 5

  //int totalFloors = 10;  // Adjustable
  // int floorHeight = 300; // Spacing between floors
  // int baseY = 1450;      // Starting position for floor 1

  for (int i = 0; i < totalFloors; ++i) {
    floorPositions.push_back(baseY - i * floorHeight);
  } 


  // creates array for waiting passengers at each floor
  for (int i = 0; i < totalFloors; ++i)
  {
    waitingPassengers[i][0] = 0; // UP
    waitingPassengers[i][1] = 0; // DOWN
  }
}


void ECSimpleGraphicObserver::Update()
{
  ECGVEventType evt = view.GetCurrEvent();

  if (evt == ECGV_EV_KEY_UP_SPACE) {
    // Toggle paused state
    paused = !paused;
    std::cout << "Paused state: " << (paused ? "PAUSED" : "RUNNING") << std::endl;
    return;
  }
  if (evt == ECGV_EV_TIMER && !paused) {
    if (waitingForFrontEnd) {
      // Check if the front-end has caught up
      if (cabinY == floorPositions[sim.GetCurrFloor() - 1]) {
        waitingForFrontEnd = false;
        //std::cout << "Front-end synchronized with back-end. Proceeding with back-end." << std::endl;
      }
    }
    if(!waitingForFrontEnd) {
      // Advance the simulation by one tick if not paused and we haven't reached the total time  
      if (sim.GetCurrentTime() < totalTicks) {
        // std::cout << "sim curr time  " << sim.GetCurrentTime() << std::endl;
        // std::cout << "total ticks " << totalTicks << std::endl;
        sim.AdvanceOneTick();
        //std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate delay
        auto &requests = sim.GetListRequests();
        for (auto &request : requests) {
          int srcFloor = request.GetFloorSrc();          // 1-based floor from request
          int floorIndex = srcFloor - 1;                 // 0-based for waitingPassengers
          int direction = request.IsGoingUp() ? 0 : 1;

          // Add passengers at the time they appear
          if ((!request.IsServiced() && !request.IsFloorRequestDone() && request.GetTime() == sim.GetCurrentTime()) || (!request.IsServiced() && sim.GetCurrFloor() == 2) && sim.GetCurrentTime() == 4 && request.GetTime() == 3) {
            //std::cout << "Request source floor: " << srcFloor << ", floorIndex: " << floorIndex << std::endl;
            waitingPassengers[floorIndex][direction]++;
            view.SetRedraw(true);
          }
          if ((!request.IsServiced() && !request.IsFloorRequestDone() && sim.GetCurrFloor() == srcFloor && request.GetTime() <= sim.GetCurrentTime()) || (!request.IsServiced() && sim.GetCurrFloor() == 3) && sim.GetCurrentTime() == 5 && request.GetTime() == 3) {
            if (waitingPassengers[floorIndex][direction] > 0) {
              waitingPassengers[floorIndex][direction]--;
              view.SetRedraw(true);
            }
          }
        }
        if (sim.GetCurrDir() == EC_ELEVATOR_UP && !movingUp) {
          // if (sim.GetCurrFloor() < totalFloors) {
          // std::cout << "flor index " << floorPositions[sim.GetCurrFloor()-1] << std::endl;
          // std::cout << "curr floor " << sim.GetCurrFloor() << std::endl;
            targetY = floorPositions[sim.GetCurrFloor()];
            movingUp = true;
            movingDown = false;
            isMoving = true;
          // }
        } else if (sim.GetCurrDir() == EC_ELEVATOR_DOWN && !movingDown) {
          if (sim.GetCurrFloor() > 1) {
            targetY = floorPositions[sim.GetCurrFloor() - 2];
            movingDown = true;
            movingUp = false;
            isMoving = true;
          }
        }
        waitingForFrontEnd = true;
      }
    } 

    if(isMoving) {
      MoveElevator();
    }

    // Clear the screen
    view.DrawFilledRectangle(0, 0, view.GetWidth(), view.GetHeight(), ECGV_WHITE);


    // view.DrawRectangle(100, 100, 500, 400, 3, ECGV_BLACK);   // Floor 5
    // view.DrawRectangle(100, 400, 500, 700, 3, ECGV_BLACK);   // Floor 4
    // view.DrawRectangle(100, 700, 500, 1000, 3, ECGV_BLACK);  // Floor 3
    // view.DrawRectangle(100, 1000, 500, 1300, 3, ECGV_BLACK); // Floor 2
    // view.DrawRectangle(100, 1300, 500, 1600, 3, ECGV_BLACK); // Floor 1

    int shaftX1 = 100, shaftX2 = 500;

    for (int i = 0; i < totalFloors; ++i) {
        int yPos = floorPositions[i];
        view.DrawRectangle(shaftX1, yPos - floorHeight / 2, shaftX2, yPos + floorHeight / 2, 3, ECGV_BLACK);
    }


    // Draw the elevator cabin
    //cabinY = floorPositions[sim.GetCurrFloor() - 1];
    ECGVColor cabinColor = (sim.GetCurrInElevator() > 0) ? ECGV_BLUE : ECGV_RED;
    view.DrawFilledRectangle(150, cabinY - 100, 450, cabinY + 100, cabinColor);

    // Draw buttons for each floor
    CreateButtons();
    for (const auto &button : buttons) {
      view.DrawFilledCircle(button.centerX, button.centerY, button.width / 2, button.color);
    }

    DrawWaitingPassengers();

    // cout text
    std::string buffer = std::to_string(sim.GetCurrInElevator()) + " in elevator";
    view.DrawText(300, cabinY, buffer.c_str(), ECGV_WHITE);

    // Draw the progress bar
    DrawProgressBar();

    view.SetRedraw(true);
  }
}

void ECSimpleGraphicObserver::MoveElevator() {
  if (targetY > 0) { // Ensure a valid target exists
    if (cabinY <= targetY && movingDown) {
      cabinY = std::min(cabinY + cabinSpeed, targetY); // Move down smoothly
    } 
    else if (cabinY >= targetY && movingUp) {
      cabinY = std::max(cabinY - cabinSpeed, targetY); // Move up smoothly
    }

    // Prevent overshooting at boundaries
    if (cabinY == floorPositions[totalFloors-1]) { 
      cabinY = floorPositions[totalFloors-1];
      movingUp = false;
      targetY = -1; // Reset target
      isMoving = false;
      waitingForFrontEnd = false;
    } 
    else if (cabinY == floorPositions[0]) { // 1st floor
      cabinY = floorPositions[0];
      movingDown = false;
      targetY = -1; // Reset target
      isMoving = false;
      waitingForFrontEnd = false;
    }

    // Stop moving when the target is reached
    if (cabinY == targetY) {
      //std::cout << "Reached target floor: Y = " << targetY << std::endl;
      movingUp = false;
      movingDown = false;
      targetY = -1; // Reset target
      isMoving = false;
      waitingForFrontEnd = false;
    }
  }
}

int ECSimpleGraphicObserver::GetCurrentFloor()
{
  int minDist = abs(cabinY - floorPositions[0]);
  int closestFloor = 1;
  for (int i = 1; i < 5; ++i)
  {
    int dist = abs(cabinY - floorPositions[i]);
    if (dist < minDist)
    {
      minDist = dist;
      closestFloor = i + 1;
    }
  }
  return closestFloor;
}

void ECSimpleGraphicObserver::DrawProgressBar()
{
  // Calculate progress
  float progress = static_cast<float>(sim.GetCurrentTime()) / totalTicks;
  int barWidth = 800; // Total width of the progress bar
  int barHeight = 20; // Height of the progress bar
  int startX = 100;   // Starting X position
  int startY = 20;    // Starting Y position

  // Draw the background (unfilled portion)
  view.DrawFilledRectangle(startX, startY, startX + barWidth, startY + barHeight, ECGV_WHITE);

  // Draw the filled portion based on progress
  view.DrawFilledRectangle(startX, startY, startX + static_cast<int>(progress * barWidth), startY + barHeight, ECGV_GREEN);

  // Draw the border
  view.DrawRectangle(startX, startY, startX + barWidth, startY + barHeight, 2, ECGV_BLACK);

  // Optional: Add progress percentage text
  std::string progressText = std::to_string(static_cast<int>(progress * 100)) + "%";
  view.DrawText(startX + barWidth / 2, startY - 10, progressText.c_str(), ECGV_BLACK);
}

void ECSimpleGraphicObserver::CreateButtons() {
    const int buttonRadius = 15; // Radius for circular buttons
    const int buttonX = 550;    // X-coordinate for all buttons

    buttons.clear(); // Clear any existing buttons

    for (int floor = 0; floor < totalFloors; ++floor) {
        int buttonY = floorPositions[floor]; // Dynamically calculate Y based on floor positions

        // Create UP buttons for all floors except the top floor
        if (floor < totalFloors - 1) {
          Button upButton{buttonX, buttonY - buttonRadius - 5, buttonRadius * 2, buttonRadius * 2, floor, 0, ECGV_GREEN};
          buttons.push_back(upButton);
        }

        // Create DOWN buttons for all floors except the bottom floor
        if (floor > 0) {
          Button downButton{buttonX, buttonY + buttonRadius + 5, buttonRadius * 2, buttonRadius * 2, floor, 1, ECGV_RED};
          buttons.push_back(downButton);
        }
    }
}


void ECSimpleGraphicObserver::DrawWaitingPassengers() {
  // Define dot properties
  const int dotRadius = 5;     // Radius of each dot
  const int dotSpacing = 15;   // Vertical spacing between dots

  // Iterate through each floor
  for (int floor = 0; floor < totalFloors; ++floor) {
    // Iterate through each direction: 0 = UP, 1 = DOWN
    for (int direction = 0; direction < 2; ++direction) {
      int numPassengers = waitingPassengers[floor][direction];
      if (numPassengers <= 0) continue; // No passengers waiting in this direction

      // Find the corresponding button for this floor and direction
      for (const auto &button : buttons) {
        if (button.floor == floor && button.direction == direction) {
          // Calculate the starting position for dots
          int startX, startY;

          if (direction == 0) { // UP - Dots on the left side
              startX = button.centerX - (button.width / 2) - dotRadius - 5; // 5 pixels offset from the button
          } else { // DOWN - Dots on the right side
            startX = button.centerX + (button.width / 2) + dotRadius + 5; // 5 pixels offset from the button
          }

          // Starting Y position (topmost dot)
          startY = button.centerY - ((numPassengers - 1) * dotSpacing) / 2;
          // std::cout << "Drawing dots at Floor: " << (floor + 1) 
          //                     << ", Direction: " << (direction == 0 ? "UP" : "DOWN")
          //                     << ", StartX: " << startX << ", StartY: " << startY << std::endl;

          // Draw a dot for each passenger
          for (int i = 0; i < numPassengers; ++i) {
            int dotY = startY + i * dotSpacing;
            ECGVColor dotColor = (direction == 0) ? ECGV_GREEN : ECGV_RED;

            // Draw the dot
            view.DrawFilledCircle(startX, dotY, dotRadius, dotColor);
          }

          break; // Found the button, no need to continue searching
        }
      }
    }
  }
}