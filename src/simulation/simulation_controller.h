#pragma once

enum class SimulationState
{
    Stopped,
    Running,
    Paused
};

// Section 8.1/8.4: owns simulation time and the Run/Pause/Stop/Step state
// machine. Circuit evaluation remains in BackendAdapter.
class SimulationController
{
public:
    static constexpr double DEFAULT_STEP_SECONDS = 0.001;
    static constexpr double MAX_FRAME_DELTA_SECONDS = 0.05;

    void run();
    void pause();
    void stop();

    double tick(double realDeltaSeconds);
    double stepOnce();

    SimulationState state() const;
    bool active() const;
    double timeSeconds() const;
    double stepSizeSeconds() const;
    void setStepSizeSeconds(double value);

private:
    SimulationState currentState = SimulationState::Stopped;
    double currentTimeSeconds = 0.0;
    double fixedStepSeconds = DEFAULT_STEP_SECONDS;
};
