#ifndef __FALLOUT_H__
#define __FALLOUT_H__

#include <memory>

typedef __int32 Fo4Int;
typedef float Fo4Float;

struct Fallout4Addresses;

class Fallout4 {
private:
	std::unique_ptr<Fallout4Addresses> addresses;

public:
	Fallout4();
	~Fallout4();

	float getShadowDistance() const;
	void setShadowDistance(float shadowDistance);
	float getShadowDirDistance() const;
	void setShadowDirDistance(float shadowDistance);
	
	int getVolumetricQuality() const;
	void setVolumetricQuality(int volumetricQuality);

	bool isPipboyActive() const;
	bool isGamePaused() const;
	bool isGameLoading() const;
};

#endif // __FALLOUT_H__
