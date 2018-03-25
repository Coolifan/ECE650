#include "exerciser.h"
#include <string>
void exercise(connection *C)
{
  query1(C, 1,35,40, 0,0,0, 0,0,0, 0,0,0, 0,0,0, 0,0,0);

  std::cout << "--------------------------------" << std::endl;
  query2(C, "Red");
    std::cout << "--------------------------------" << std::endl;
  query3(C, "Duke");
    std::cout << "--------------------------------" << std::endl;
  query4(C, "NC", "LightBlue");
  std::cout << "--------------------------------" << std::endl;
  query5(C, 10);
}
