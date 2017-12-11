#include <memory>
#include <sstream>
#include <string>

#include "Includes.h"
#include "Controller.h"
#include "Console.h"
#include "Fallout.h"

namespace
{
    const double micros = 1.0E6;

    float GetPrivateProfileFloatA(const LPCSTR lpAppName, const LPCSTR lpKeyName, const float defaultValue, const LPCSTR lpFileName)
    {
        char result[256];
        const auto length = GetPrivateProfileStringA(lpAppName, lpKeyName, "", result, 256, lpFileName);
        if (length == 0)
        {
            return defaultValue;
        }
        _set_errno(0);
        const float parsed = float(atof(result));
        if (errno)
        {
            throw std::exception(
                (std::string("Unable to parse ") + lpKeyName + "=" + std::string(result, length)).c_str());
        }

        return parsed;
    }

    struct Config
    {
        bool capFramerate;
        float targetLoad;
        float minDistance;
        float maxDistance;
        float momentumDownAcceleration;
        float minMomentum;
        float momentumUpAcceleration;
        float maxMomentum;
        bool adjustVolumetricQuality;
        bool showDiagnostics;
        float columetricQualityDistances[3];
        bool loadCapping;
        bool usePreciseSleep;
        microsecond_t debugMessageDelay;
        microsecond_t targetFrameTime;
        int loadingSyncIntervalOverride;
        int pausedSyncIntervalOverride;
        int syncIntervalOverride;
    };

    const char* GetVSyncString(const int syncIntervalOverride)
    {
        return (syncIntervalOverride == -1 ? "Default" : syncIntervalOverride == 0 ? "Disabled" : "Enabled");
    }
}

template <typename value_t, unsigned int maxRecords>
class Average
{
private:
    value_t values[maxRecords];
    int currentOffset;
    int totalRecords;

public:
    Average() : values(), currentOffset(0), totalRecords(0)
    {
    }

    Average& operator +=(value_t value)
    {
        values[currentOffset] = value;
        if (++currentOffset >= maxRecords)
        {
            currentOffset = 0;
        }
        if (totalRecords < maxRecords)
        {
            ++totalRecords;
        }

        return *this;
    }

    operator value_t() const
    {
        value_t total(0);
        for (int i = 0; i < totalRecords; ++i)
        {
            total += values[i];
        }
        return total / totalRecords;
    }
};

class Clock
{
private:
    double timeToMicros;
public:
    Clock()
    {
        LARGE_INTEGER _frequency;
        timeBeginPeriod(1);
        QueryPerformanceFrequency(&_frequency);
        timeToMicros = micros / _frequency.QuadPart;
    }

    double getMicrosPerTick() const
    {
        return timeToMicros;
    }

    microsecond_t currentMicrosecond() const
    {
        LARGE_INTEGER time;
        QueryPerformanceCounter(&time);

        return microsecond_t(time.QuadPart * timeToMicros);
    }

    void sleepUntil(const microsecond_t time) const
    {
        const microsecond_t timeRemaining = time - currentMicrosecond();
        const millisecond_t sleepTimeInt = millisecond_t(timeRemaining / 1000);
        if (sleepTimeInt <= 0)
        {
            return;
        }
        Sleep(DWORD(sleepTimeInt));
    }

    void preciseSleepUntil(const microsecond_t time) const
    {
        sleepUntil(time);
        while (currentMicrosecond() < time)
        {
        }
    }
} Clock;


class AdvancedController
{
private:
    const std::shared_ptr<Fallout4> fallout4;
    const Config config;
    float momentum;
    float load;
    Average<float, 10> workTimeAverage;

    microsecond_t lastFrameStart;
    microsecond_t workCompleted;
    microsecond_t nextDebugTime;
    bool actualVSync;
    bool requestedVSync;

    bool shouldCap() const
    {
        return config.capFramerate && (config.loadCapping || !fallout4->isGameLoading());
    }

    bool shouldAdjust() const
    {
        return !(fallout4->isMainMenu() || fallout4->isGamePaused() || fallout4->isGameLoading());
    }


    void adjust()
    {
        if (config.targetLoad > load)
        {
            if (momentum < 0)
            {
                momentum = 1;
            }
            else
            {
                momentum = min(momentum * config.momentumUpAcceleration, config.maxMomentum);
            }
        }
        else
        {
            if (momentum > 0)
            {
                momentum = -2;
            }
            else
            {
                momentum = max(momentum * config.momentumDownAcceleration, config.minMomentum);
            }
        }
        const float newDistance = max(config.minDistance, min(config.maxDistance, fallout4->getShadowDirDistance() +
            momentum));
        fallout4->setShadowDirDistance(newDistance);
        if (config.adjustVolumetricQuality)
        {
            fallout4->setVolumetricQuality(getVolumetricQuality(newDistance));
        }
    }

    int getVolumetricQuality(const float distance) const
    {
        for (int i = 0; i < 3; ++i)
        {
            if (distance < config.columetricQualityDistances[i])
            {
                return i;
            }
        }
        return 3;
    }

    bool shouldDisplayDebug() const
    {
        return config.showDiagnostics && nextDebugTime < lastFrameStart;
    }

    int getSyncIntervalOverride() const
    {
        return
            fallout4->isGameLoading()
                ? config.loadingSyncIntervalOverride
                : fallout4->isGamePaused()
                ? config.pausedSyncIntervalOverride
                : config.syncIntervalOverride;
    }

public:
    AdvancedController(const std::shared_ptr<Fallout4> &fallout4, const Config &config) :
        fallout4(fallout4),
        config(config),
        momentum(1),
        load(0),
        lastFrameStart(0), 
        workCompleted(0),
        nextDebugTime(0), 
        actualVSync(false)
    {
    }

    void prePresent(UINT& SyncInterval, UINT& Flags)
    {
        requestedVSync = SyncInterval != 0;

        const int syncOverride = getSyncIntervalOverride();
        if (syncOverride >= 0)
        {
            SyncInterval = syncOverride;
        }
        actualVSync = SyncInterval != 0;
        workCompleted = Clock.currentMicrosecond();
    }

    void postPresent()
    {
        if (lastFrameStart == 0)
        {
            lastFrameStart = Clock.currentMicrosecond();
            return;
        }

        const microsecond_t currentWorkTime = workCompleted - lastFrameStart;

        workTimeAverage += float(currentWorkTime);

        load = float(workTimeAverage) / config.targetFrameTime;

        if (shouldAdjust())
        {
            adjust();
        }

        microsecond_t sleepTime = 0;

        if (shouldCap())
        {
            sleepTime = lastFrameStart + config.targetFrameTime - Clock.currentMicrosecond();
            if (config.usePreciseSleep)
            {
                Clock.preciseSleepUntil(lastFrameStart + config.targetFrameTime);
            }
            else
            {
                Clock.sleepUntil(lastFrameStart + config.targetFrameTime);
            }
        }

        if (shouldDisplayDebug())
        {
            const microsecond_t totalFrameTime = Clock.currentMicrosecond() - lastFrameStart;
            char buf[256];
            sprintf_s(buf, sizeof(buf),
                      "%6d+=%+7.1f:V%d:%5.1fms + %5.1fms = %5.1fms:%3.0f%%:%5s:%4s:%4s %c %-7s\n dist +=momentm:gr:   work +   sleep =  frame :load:     :    :Game O Actual\r",
                      int(fallout4->getShadowDirDistance()), momentum, fallout4->getVolumetricQuality(),
                      workTimeAverage / 1000.0f,
                      sleepTime / 1000.0f, totalFrameTime / 1000.0f, load * 100,
                      fallout4->isGamePaused() != 0 ? "Pause" : "",
                      fallout4->isGameLoading() != 0 ? "Load" : "",
                      requestedVSync ? "Sync" : "None", getSyncIntervalOverride() < 0 ? '*' : ' ', actualVSync ? "Sync" : "None"
            );
            console.print(buf);
            nextDebugTime = Clock.currentMicrosecond() + config.debugMessageDelay;
        }

        lastFrameStart = Clock.currentMicrosecond();
    }
};

std::unique_ptr<AdvancedController> make_controller(const std::string& configFile,
                                                    const std::shared_ptr<Fallout4>& fallout4)
{
    auto configPath = configFile.c_str();
    Config config;
    const auto targetFPS = GetPrivateProfileFloatA("Simple", "fTargetFPS", 60.0, configPath);
    config.targetFrameTime = microsecond_t(micros / targetFPS);
    config.capFramerate = GetPrivateProfileIntA("Simple", "bCapFramerate", TRUE, configPath) != FALSE;
    const auto targetLoadPercentage = GetPrivateProfileFloatA("Simple", "fTargetLoad", 98, configPath);
    config.targetLoad = targetLoadPercentage / 100;
    config.minDistance = max(0.0f, GetPrivateProfileFloatA("Simple", "fShadowDirDistanceMin", 2500.0f, configPath));
    config.maxDistance = max(0.0f, GetPrivateProfileFloatA("Simple", "fShadowDirDistanceMax", 12000.0f, configPath));
    config.momentumDownAcceleration = abs(
        GetPrivateProfileFloatA("Simple", "fMomentumDownAcceleration", 1.1, configPath));
    config.minMomentum = -abs(GetPrivateProfileFloatA("Simple", "fMinMomentum", 1000.0f, configPath));
    config.momentumUpAcceleration =
        abs(GetPrivateProfileFloatA("Simple", "fMomentumUpAcceleration", 1.025, configPath));;
    config.maxMomentum = abs(GetPrivateProfileFloatA("Simple", "fMaxMomentum", 500.0f, configPath));
    config.adjustVolumetricQuality = GetPrivateProfileIntA("Simple", "bAdjustGRQuality", TRUE, configPath) != FALSE;
    config.showDiagnostics = GetPrivateProfileIntA("Advanced", "bShowDiagnostics", FALSE, configPath) != FALSE;
    config.columetricQualityDistances[0] = GetPrivateProfileFloatA("Simple", "fGRQualityShadowDist1", 3000, configPath);
    config.columetricQualityDistances[1] = GetPrivateProfileFloatA("Simple", "fGRQualityShadowDist2", 6000, configPath);
    config.columetricQualityDistances[2] =
        GetPrivateProfileFloatA("Simple", "fGRQualityShadowDist3", 10000, configPath);
    config.loadCapping = GetPrivateProfileIntA("Advanced", "bLoadCapping", FALSE, configPath) != FALSE;
    config.usePreciseSleep = GetPrivateProfileIntA("Advanced", "bUsePreciseCapping", FALSE, configPath) != FALSE;
    config.debugMessageDelay = microsecond_t(
        micros / GetPrivateProfileFloatA("Advanced", "fDebugMessageFrequency", 1.0f, configPath));
    config.syncIntervalOverride = GetPrivateProfileIntA("Advanced", "iPresentIntervalOverride", -1, configPath);
    config.loadingSyncIntervalOverride = GetPrivateProfileIntA("Advanced", "iLoadingPresentIntervalOverride",
                                                               config.loadCapping ? config.syncIntervalOverride : 0,
                                                               configPath);
    config.pausedSyncIntervalOverride = GetPrivateProfileIntA("Advanced", "iPausedPresentIntervalOverride",
                                                              config.syncIntervalOverride, configPath);

    if (config.showDiagnostics)
    {
        console.print("Dynamic Performance Tuner is running.\n");
        std::stringstream details;
        details
            << "  Target FPS:            " << targetFPS << std::endl
            << "  Framerate:             " << (config.capFramerate ? "Locked" : "Unlocked") << std::endl
            << "  V-Sync:" << std::endl
            << "    When Paused:         " << GetVSyncString(config.pausedSyncIntervalOverride) << std::endl
            << "    When Loading:        " << GetVSyncString(config.loadingSyncIntervalOverride) << std::endl
            << "    Otherwise:           " << GetVSyncString(config.syncIntervalOverride) << std::endl
            << "  Sleep precision:       " << (config.usePreciseSleep ? "Microsecond" : "Millisecond") << std::endl
            << "  Target Load:           " << targetLoadPercentage << "%" << std::endl
            << "  Load Screen:           " << (config.loadCapping ? "Normal" : "Accelerated") << std::endl
            << "  Shadow Distance Range: (" << config.minDistance << " to " << config.maxDistance << ")" << std::endl
            << "  Shadow Momentum Range: (" << config.minMomentum << " to " << config.maxMomentum << ")" << std::endl
            << "  Momentum Accelaration: (" << config.momentumDownAcceleration << " downward, " << config.momentumUpAcceleration << " upward)" << std::endl
            << "  Godray Quality:        " << (config.adjustVolumetricQuality ? "Adjusting" : "Constant") << std::endl;

        if (config.adjustVolumetricQuality)
        {
            for (int i = 0; i < 3; ++i)
            {
                details << "   Quality Level " << (i + 1) << " Distance: " << config.columetricQualityDistances[i] <<
                    std::endl;
            }
        }
        console.print(details.str());
    }

    return std::make_unique<AdvancedController>(fallout4, config);
}

Controller::Controller(const std::string& configPath, const std::shared_ptr<Fallout4>& fallout4)
    : controllerImpl(make_controller(configPath, fallout4))
{
}

Controller::~Controller()
{
}

void Controller::postPresent() const
{
    controllerImpl->postPresent();
}

void Controller::prePresent(UINT& SyncInterval, UINT& Flags) const
{
    controllerImpl->prePresent(SyncInterval, Flags);
}
