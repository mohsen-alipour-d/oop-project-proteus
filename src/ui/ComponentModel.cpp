#include "ComponentModel.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>

bool WorldRect::contains(const WorldPoint& point) const
{
    return point.x >= x && point.x <= x + w &&
           point.y >= y && point.y <= y + h;
}

bool WorldRect::intersects(const WorldRect& other) const
{
    return x <= other.x + other.w && x + w >= other.x &&
           y <= other.y + other.h && y + h >= other.y;
}

std::string toUpperAscii(const std::string& text)
{
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char character)
                   {
                       return static_cast<char>(std::toupper(character));
                   });
    return result;
}

std::string trimAscii(const std::string& text)
{
    const auto first = std::find_if_not(text.begin(), text.end(),
                                        [](unsigned char character)
                                        {
                                            return std::isspace(character) != 0;
                                        });

    if (first == text.end())
    {
        return {};
    }

    const auto last = std::find_if_not(text.rbegin(), text.rend(),
                                       [](unsigned char character)
                                       {
                                           return std::isspace(character) != 0;
                                       }).base();

    return std::string(first, last);
}

ComponentLibrary::ComponentLibrary()
{
    categoryNames = {"ALL", "ANALOG", "SOURCES", "DIGITAL", "OUTPUTS",
                     "ADVANCED", "MEASUREMENT"};

    const auto sidePins = [](int leftCount, int rightCount,
                             double halfWidth, double spacing)
    {
        std::vector<PinDefinition> pins;
        for (int index = 0; index < leftCount; ++index)
        {
            const double y = (index - (leftCount - 1) / 2.0) * spacing;
            pins.push_back({"L" + std::to_string(index), {-halfWidth, y}});
        }
        for (int index = 0; index < rightCount; ++index)
        {
            const double y = (index - (rightCount - 1) / 2.0) * spacing;
            pins.push_back({"R" + std::to_string(index), {halfWidth, y}});
        }
        return pins;
    };

    const auto memoryPins = []()
    {
        std::vector<PinDefinition> pins;
        for (int index = 0; index < 8; ++index)
        {
            pins.push_back({"A" + std::to_string(index),
                            {-71.0, (index - 3.5) * 16.0}});
        }
        for (int index = 0; index < 8; ++index)
        {
            pins.push_back({"D" + std::to_string(index),
                            {71.0, (index - 3.5) * 16.0}});
        }
        pins.push_back({"RD", {-71.0, 64.0}});
        pins.push_back({"WR", {-71.0, 78.0}});
        return pins;
    };

    definitions = {
        {0, "RESISTOR", "ANALOG", "R", "1K", "RESISTANCE",
         SymbolKind::Resistor, 90.0, 42.0,
         {{"1", {-45.0, 0.0}}, {"2", {45.0, 0.0}}}},

        {1, "CAPACITOR", "ANALOG", "C", "100N", "CAPACITANCE",
         SymbolKind::Capacitor, 80.0, 52.0,
         {{"1", {-40.0, 0.0}}, {"2", {40.0, 0.0}}}},

        {2, "BATTERY", "SOURCES", "BAT", "9V", "VOLTAGE",
         SymbolKind::Battery, 82.0, 60.0,
         {{"-", {-41.0, 0.0}}, {"+", {41.0, 0.0}}}},

        {3, "CLOCK", "SOURCES", "CLK", "1HZ", "FREQUENCY",
         SymbolKind::Clock, 82.0, 58.0,
         {{"OUT", {41.0, 0.0}}, {"GND", {-41.0, 0.0}}}},

        {4, "SWITCH", "DIGITAL", "SW", "OPEN", "STATE",
         SymbolKind::Switch, 90.0, 48.0,
         {{"1", {-45.0, 0.0}}, {"2", {45.0, 0.0}}}},

        {5, "AND GATE", "DIGITAL", "U", "74HC08", "MODEL",
         SymbolKind::AndGate, 92.0, 70.0,
         {{"A", {-46.0, -18.0}}, {"B", {-46.0, 18.0}}, {"Y", {46.0, 0.0}}}},

        {6, "LED", "OUTPUTS", "D", "RED", "COLOR",
         SymbolKind::Led, 86.0, 58.0,
         {{"A", {-43.0, 0.0}}, {"K", {43.0, 0.0}}}},

        {7, "GROUND", "SOURCES", "GND", "0V", "REFERENCE",
         SymbolKind::Ground, 54.0, 62.0,
         {{"GND", {0.0, -31.0}}}},

        {8, "INDUCTOR", "ANALOG", "L", "1M", "INDUCTANCE",
         SymbolKind::Inductor, 90.0, 42.0,
         {{"1", {-45.0, 0.0}}, {"2", {45.0, 0.0}}}},

        {9, "DC SOURCE", "SOURCES", "V", "5V", "VOLTAGE",
         SymbolKind::GenericBlock, 90.0, 64.0,
         {{"-", {-45.0, 0.0}}, {"+", {45.0, 0.0}}}},

        {10, "PUSH BUTTON", "DIGITAL", "PB", "RELEASED", "STATE",
         SymbolKind::GenericBlock, 94.0, 58.0,
         {{"OUT", {47.0, 0.0}}}},

        {11, "7 SEGMENT", "OUTPUTS", "SEG", "0", "DISPLAY",
         SymbolKind::GenericBlock, 98.0, 104.0, sidePins(8, 0, 49.0, 11.0)},

        {12, "OR GATE", "DIGITAL", "U", "2 INPUTS", "MODEL",
         SymbolKind::GenericBlock, 92.0, 70.0,
         {{"A", {-46.0, -18.0}}, {"B", {-46.0, 18.0}}, {"Y", {46.0, 0.0}}}},

        {13, "NOT GATE", "DIGITAL", "U", "INVERTER", "MODEL",
         SymbolKind::GenericBlock, 86.0, 60.0,
         {{"A", {-43.0, 0.0}}, {"Y", {43.0, 0.0}}}},

        {14, "XOR GATE", "DIGITAL", "U", "2 INPUTS", "MODEL",
         SymbolKind::GenericBlock, 92.0, 70.0,
         {{"A", {-46.0, -18.0}}, {"B", {-46.0, 18.0}}, {"Y", {46.0, 0.0}}}},

        {15, "NAND GATE", "DIGITAL", "U", "2 INPUTS", "MODEL",
         SymbolKind::GenericBlock, 92.0, 70.0,
         {{"A", {-46.0, -18.0}}, {"B", {-46.0, 18.0}}, {"Y", {46.0, 0.0}}}},

        {16, "D FLIP FLOP", "DIGITAL", "FF", "D TYPE", "MODEL",
         SymbolKind::GenericBlock, 98.0, 76.0,
         {{"D", {-49.0, -18.0}}, {"CLK", {-49.0, 18.0}}, {"Q", {49.0, 0.0}}}},

        {17, "ADC", "ADVANCED", "ADC", "4 BIT", "RESOLUTION",
         SymbolKind::GenericBlock, 112.0, 112.0, sidePins(3, 4, 56.0, 20.0)},

        {18, "DAC", "ADVANCED", "DAC", "4 BIT", "RESOLUTION",
         SymbolKind::GenericBlock, 112.0, 124.0, sidePins(6, 1, 56.0, 18.0)},

        {19, "MICROCONTROLLER", "ADVANCED", "MCU", "1US", "INSTRUCTION TIME",
         SymbolKind::GenericBlock, 132.0, 132.0, sidePins(8, 8, 66.0, 14.0)},

        {20, "EXTERNAL MEMORY", "ADVANCED", "MEM", "8 BIT", "ADDRESS WIDTH",
         SymbolKind::GenericBlock, 142.0, 172.0, memoryPins()},

        {21, "LCD 16X2", "OUTPUTS", "LCD", "16X2", "DISPLAY",
         SymbolKind::GenericBlock, 142.0, 130.0, sidePins(11, 0, 71.0, 11.0)},

        {22, "MATRIX KEYPAD", "ADVANCED", "KEY", "4X4", "SIZE",
         SymbolKind::GenericBlock, 112.0, 104.0, sidePins(4, 4, 56.0, 18.0)},

        {23, "VOLTMETER", "MEASUREMENT", "VM", "AUTO", "RANGE",
         SymbolKind::GenericBlock, 96.0, 64.0,
         {{"+", {-48.0, 0.0}}, {"-", {48.0, 0.0}}}},

        {24, "AMMETER", "MEASUREMENT", "AM", "AUTO", "RANGE",
         SymbolKind::GenericBlock, 96.0, 64.0,
         {{"+", {-48.0, 0.0}}, {"-", {48.0, 0.0}}}}
    };
}

const ComponentDefinition* ComponentLibrary::findById(int id) const
{
    const auto found = std::find_if(definitions.begin(), definitions.end(),
                                    [id](const ComponentDefinition& definition)
                                    {
                                        return definition.id == id;
                                    });

    return found == definitions.end() ? nullptr : &(*found);
}

std::vector<int> ComponentLibrary::filteredIds(const std::string& category,
                                                const std::string& query) const
{
    const std::string normalizedCategory = toUpperAscii(trimAscii(category));
    const std::string normalizedQuery = toUpperAscii(trimAscii(query));
    std::vector<int> result;

    for (const ComponentDefinition& definition : definitions)
    {
        const bool categoryMatches = normalizedCategory.empty() ||
                                     normalizedCategory == "ALL" ||
                                     definition.category == normalizedCategory;

        const bool queryMatches = normalizedQuery.empty() ||
                                  definition.name.find(normalizedQuery) != std::string::npos ||
                                  definition.category.find(normalizedQuery) != std::string::npos;

        if (categoryMatches && queryMatches)
        {
            result.push_back(definition.id);
        }
    }

    return result;
}

const std::vector<std::string>& ComponentLibrary::categories() const
{
    return categoryNames;
}

ComponentInstance::ComponentInstance(int newInstanceId,
                                     int newDefinitionId,
                                     const std::string& newLabel,
                                     const std::string& newValue,
                                     const WorldPoint& newPosition)
        : instanceId(newInstanceId),
          componentDefinitionId(newDefinitionId),
          componentLabel(newLabel),
          componentValue(newValue),
          componentPosition(newPosition)
{
}

int ComponentInstance::id() const
{
    return instanceId;
}

int ComponentInstance::definitionId() const
{
    return componentDefinitionId;
}

const std::string& ComponentInstance::label() const
{
    return componentLabel;
}

void ComponentInstance::setLabel(const std::string& newLabel)
{
    componentLabel = newLabel;
}

const std::string& ComponentInstance::value() const
{
    return componentValue;
}

void ComponentInstance::setValue(const std::string& newValue)
{
    componentValue = newValue;
}

const WorldPoint& ComponentInstance::position() const
{
    return componentPosition;
}

void ComponentInstance::setPosition(const WorldPoint& newPosition)
{
    componentPosition = newPosition;
}

int ComponentInstance::rotationDegrees() const
{
    return rotationQuarterTurns * 90;
}

bool ComponentInstance::isMirroredHorizontally() const
{
    return mirroredHorizontally;
}

bool ComponentInstance::isMirroredVertically() const
{
    return mirroredVertically;
}

void ComponentInstance::rotateClockwise()
{
    rotationQuarterTurns = (rotationQuarterTurns + 1) % 4;
}

void ComponentInstance::mirrorHorizontally()
{
    mirroredHorizontally = !mirroredHorizontally;
}

void ComponentInstance::mirrorVertically()
{
    mirroredVertically = !mirroredVertically;
}

void ComponentInstance::setTransform(int rotationDegrees,
                                     bool mirrorHorizontal,
                                     bool mirrorVertical)
{
    int normalized = rotationDegrees % 360;
    if (normalized < 0)
    {
        normalized += 360;
    }
    rotationQuarterTurns = (normalized / 90) % 4;
    mirroredHorizontally = mirrorHorizontal;
    mirroredVertically = mirrorVertical;
}

WorldPoint ComponentInstance::transformLocalPoint(const WorldPoint& localPoint) const
{
    // Mirroring around the horizontal axis changes Y. Mirroring around the
    // vertical axis changes X. The rotation is applied afterwards.
    double x = mirroredVertically ? -localPoint.x : localPoint.x;
    double y = mirroredHorizontally ? -localPoint.y : localPoint.y;

    double rotatedX = x;
    double rotatedY = y;

    switch (rotationQuarterTurns)
    {
        case 1:
            rotatedX = -y;
            rotatedY = x;
            break;

        case 2:
            rotatedX = -x;
            rotatedY = -y;
            break;

        case 3:
            rotatedX = y;
            rotatedY = -x;
            break;

        default:
            break;
    }

    return {componentPosition.x + rotatedX,
            componentPosition.y + rotatedY};
}

WorldPoint ComponentInstance::pinPosition(const ComponentDefinition& definition,
                                          std::size_t pinIndex) const
{
    if (pinIndex >= definition.pins.size())
    {
        return componentPosition;
    }

    return transformLocalPoint(definition.pins[pinIndex].localPosition);
}

WorldRect ComponentInstance::bounds(const ComponentDefinition& definition) const
{
    const double halfWidth = definition.width / 2.0;
    const double halfHeight = definition.height / 2.0;
    const WorldPoint corners[] = {
        {-halfWidth, -halfHeight},
        { halfWidth, -halfHeight},
        { halfWidth,  halfHeight},
        {-halfWidth,  halfHeight}
    };

    double minimumX = std::numeric_limits<double>::max();
    double minimumY = std::numeric_limits<double>::max();
    double maximumX = std::numeric_limits<double>::lowest();
    double maximumY = std::numeric_limits<double>::lowest();

    for (const WorldPoint& corner : corners)
    {
        const WorldPoint transformed = transformLocalPoint(corner);
        minimumX = std::min(minimumX, transformed.x);
        minimumY = std::min(minimumY, transformed.y);
        maximumX = std::max(maximumX, transformed.x);
        maximumY = std::max(maximumY, transformed.y);
    }

    return {minimumX, minimumY, maximumX - minimumX, maximumY - minimumY};
}
