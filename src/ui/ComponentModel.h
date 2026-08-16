#pragma once

#include <string>
#include <vector>

struct WorldPoint
{
    double x = 0.0;
    double y = 0.0;
};

struct WorldRect
{
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;

    bool contains(const WorldPoint& point) const;
    bool intersects(const WorldRect& other) const;
};

enum class SymbolKind
{
    Resistor,
    Capacitor,
    Battery,
    Led,
    Ground,
    Switch,
    AndGate,
    Clock,
    Inductor,
    GenericBlock
};

struct PinDefinition
{
    std::string name;
    WorldPoint localPosition;
};

struct ComponentDefinition
{
    int id = -1;
    std::string name;
    std::string category;
    std::string prefix;
    std::string defaultValue;
    std::string valueCaption;
    SymbolKind symbol = SymbolKind::Resistor;
    double width = 80.0;
    double height = 50.0;
    std::vector<PinDefinition> pins;
};

class ComponentLibrary
{
public:
    ComponentLibrary();

    const ComponentDefinition* findById(int id) const;
    std::vector<int> filteredIds(const std::string& category,
                                 const std::string& query) const;
    const std::vector<std::string>& categories() const;

private:
    std::vector<ComponentDefinition> definitions;
    std::vector<std::string> categoryNames;
};

class ComponentInstance
{
public:
    ComponentInstance(int newInstanceId,
                      int newDefinitionId,
                      const std::string& newLabel,
                      const std::string& newValue,
                      const WorldPoint& newPosition);

    int id() const;
    int definitionId() const;

    const std::string& label() const;
    void setLabel(const std::string& newLabel);

    const std::string& value() const;
    void setValue(const std::string& newValue);

    const WorldPoint& position() const;
    void setPosition(const WorldPoint& newPosition);

    int rotationDegrees() const;
    bool isMirroredHorizontally() const;
    bool isMirroredVertically() const;

    void rotateClockwise();
    void mirrorHorizontally();
    void mirrorVertically();
    void setTransform(int rotationDegrees,
                      bool mirrorHorizontal,
                      bool mirrorVertical);

    WorldPoint transformLocalPoint(const WorldPoint& localPoint) const;
    WorldPoint pinPosition(const ComponentDefinition& definition,
                           std::size_t pinIndex) const;
    WorldRect bounds(const ComponentDefinition& definition) const;

private:
    int instanceId;
    int componentDefinitionId;
    std::string componentLabel;
    std::string componentValue;
    WorldPoint componentPosition;
    int rotationQuarterTurns = 0;
    bool mirroredHorizontally = false;
    bool mirroredVertically = false;
};

std::string toUpperAscii(const std::string& text);
std::string trimAscii(const std::string& text);
