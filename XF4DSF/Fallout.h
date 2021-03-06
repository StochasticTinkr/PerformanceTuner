#ifndef __FALLOUT_H__
#define __FALLOUT_H__

#include <memory>
#include <string>

typedef __int32 Fo4Int;
typedef float Fo4Float;

struct Fallout4Addresses;

class Fallout4 {
private:
	std::unique_ptr<Fallout4Addresses> addresses;

public:
	Fallout4(std::string executableFile);
	~Fallout4();

	float getShadowDirDistance() const;
	void setShadowDirDistance(float shadowDistance);
	
	int getVolumetricQuality() const;
	void setVolumetricQuality(int volumetricQuality);

	bool isGamePaused() const;
	bool isGameLoading() const;
	bool isMainMenu() const;
	bool loadedSuccessfully() const;
};

#endif // __FALLOUT_H__
