#include "simulation_controller.h"

#include <algorithm>

void SimulationController::run()
{
    currentState = SimulationState::Running;
}

void SimulationController::pause()
{
    if (currentState != SimulationState::Stopped)
    {
        currentState = SimulationState::Paused;
    }
}

void SimulationController::stop()
{
    currentState = SimulationState::Stopped;
    currentTimeSeconds = 0.0;
}

double SimulationController::tick(double realDeltaSeconds)
{
    if (currentState != SimulationState::Running || realDeltaSeconds <= 0.0)
    {
        return 0.0;
    }

    const double delta = std::min(realDeltaSeconds,
                                  MAX_FRAME_DELTA_SECONDS);
    currentTimeSeconds += delta;
    return delta;
}

double SimulationController::stepOnce()
{
    currentState = SimulationState::Paused;
    currentTimeSeconds += fixedStepSeconds;
    return fixedStepSeconds;
}

SimulationState SimulationController::state() const
{
    return currentState;
}

bool SimulationController::active() const
{
    return currentState != SimulationState::Stopped;
}

double SimulationController::timeSeconds() const
{
    return currentTimeSeconds;
}

double SimulationController::stepSizeSeconds() const
{
    return fixedStepSeconds;
}

void SimulationController::setStepSizeSeconds(double value)
{
    if (value > 0.0)
    {
        fixedStepSeconds = value;
    }
}
