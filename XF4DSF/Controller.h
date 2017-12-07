#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include <memory>
#include "Fallout.h"

typedef __int64 microsecond_t;
typedef __int64 millisecond_t;

class AdvancedController;

class Controller
{
private:
	std::unique_ptr<AdvancedController> controllerImpl;
public:
	Controller(std::string configPath, std::shared_ptr<Fallout4> fallout4);
	~Controller();
	void tick();
};

#endif // __CONTROLLER_H__
