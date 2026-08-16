#pragma once

#include "../ui/ComponentModel.h"
#include "../core/circuit.h"
#include "../file/file_manager.h"
#include "../file/history.h"
#include "../drc/drc.h"
#include "../measurement/oscilloscope.h"
#include "../simulation/simulation_controller.h"

#include <optional>
#include <string>
#include <vector>

enum class WireLogicState
{
    Low,
    High,
    Undefined,
    Floating
};

struct PinHandle
{
    int componentId = -1;
    int pinIndex = -1;

    bool valid() const
    {
        return componentId >= 0 && pinIndex >= 0;
    }

    bool operator==(const PinHandle& other) const
    {
        return componentId == other.componentId && pinIndex == other.pinIndex;
    }

    bool operator!=(const PinHandle& other) const
    {
        return !(*this == other);
    }
};

struct WireView
{
    int id = -1;
    std::vector<WorldPoint> points;
    WireLogicState state = WireLogicState::Floating;
    double voltage = 0.0;
    bool driven = false;
};

struct ScopeChannelView
{
    int channelIndex = -1;
    int netId = -1;
    double timeDiv = 1.0;
    double voltDiv = 1.0;
    std::vector<double> history;
};

class BackendAdapter
{
public:
    BackendAdapter();

    void resetProject();
    int addComponent(int definitionId,
                     const std::string& label,
                     const std::string& value,
                     const WorldPoint& position);
    bool syncComponent(const ComponentInstance& component);
    bool removeComponent(int componentId);
    bool isLabelAvailable(const std::string& label, int exceptId) const;

    std::optional<PinHandle> findPinAt(const WorldPoint& point,
                                       double tolerance) const;
    std::optional<WorldPoint> pinPosition(const PinHandle& handle) const;
    bool connectPins(const PinHandle& first, const PinHandle& second);
    int findWireAt(const WorldPoint& point, double tolerance) const;
    bool removeWire(int wireId);
    bool addJunctionAt(const WorldPoint& point);
    std::vector<WireView> wireViews() const;
    std::vector<WorldPoint> junctionPositions() const;

    bool attachOscilloscopeChannel(int channelIndex, int wireId);
    void startOscilloscope();
    void pauseOscilloscope();
    std::vector<ScopeChannelView> oscilloscopeChannels() const;

    void recordHistory();
    bool undo();
    bool redo();
    bool canUndo() const;
    bool canRedo() const;

    bool save(const std::string& defaultPath = "proteus_project.txt");
    bool load(const std::string& path = "proteus_project.txt");
    bool loadMostRecent();
    std::string currentPath() const;
    std::vector<ComponentInstance> frontendComponents() const;

    bool validate();
    const std::vector<LogMessage>& validationMessages() const;

    void startSimulation();
    void pauseSimulation();
    void updateSimulation(double dt);
    bool stepSimulation();
    void step(double dt);
    void stopSimulation();
    SimulationState simulationState() const;
    double simulationTimeSeconds() const;
    double simulationStepSeconds() const;

    bool toggleSwitch(int componentId);
    bool setPushButtonPressed(int componentId, bool pressed);
    bool adjustInteractiveValue(int componentId, int direction);
    std::string runtimeComponentValue(int componentId) const;

private:
    Circuit circuit;
    FileManager fileManager;
    History history;
    DRC drc;
    Oscilloscope oscilloscope;
    SimulationController simulationController;
    ComponentLibrary definitions;
    std::string simulationBaseline;

    Component* componentById(int componentId) const;
    Wire* wireById(int wireId) const;
    Pin* pinFromHandle(const PinHandle& handle) const;
    Component* createBackendComponent(int definitionId,
                                      const std::string& label,
                                      const std::string& value,
                                      const WorldPoint& position) const;
    int definitionIdFor(const Component& component) const;
    std::string displayValueFor(const Component& component,
                                int definitionId) const;
    void applyDefinitionGeometry(Component& component, int definitionId) const;
    void applyEditableValue(Component& component,
                            int definitionId,
                            const std::string& value) const;
    void prepareSimulationSession();
    void evaluateCircuit(double dt);
};
