#include "ECGraphicViewImp.h"
#include "SimpleObserver.h"

// Test graphical view code
int real_main(int argc, char **argv)
{
    const int widthWin = 1000, heightWin = 1750;
    ECGraphicViewImp view(widthWin, heightWin);
  
    // create a simple observer
    ECSimpleGraphicObserver obs(view);
    view.Attach(&obs);
    
    view.Show();
  
  
    return 0;
}

int main(int argc, char **argv)
{
    return real_main(argc, argv);
    //return al_run_main(argc, argv, real_main);
}

