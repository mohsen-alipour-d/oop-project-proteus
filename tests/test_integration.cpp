#include <cstdio>
#include <iostream>
#include <vector>

#include "integration/backend_adapter.h"

namespace
{
int failures = 0;

void check(bool condition, const std::string& name)
{
    if (condition)
    {
        std::cout << "[PASS] " << name << '\n';
    }
    else
    {
        std::cout << "[FAIL] " << name << '\n';
        ++failures;
    }
}
}

int main()
{
    BackendAdapter backend;

    const int ground = backend.addComponent(7, "GND1", "0V", {0.0, 80.0});
    const int source = backend.addComponent(2, "BAT1", "5V", {0.0, 0.0});
    const int resistor = backend.addComponent(0, "R1", "1K", {100.0, 0.0});

    check(ground >= 0 && source >= 0 && resistor >= 0,
          "frontend definitions create backend components");
    check(backend.addComponent(0, "R1", "2K", {200.0, 0.0}) < 0,
          "duplicate labels are rejected");
    check(backend.connectPins({source, 0}, {resistor, 0}),
          "first wire is connected");
    check(backend.connectPins({ground, 0}, {resistor, 1}),
          "second wire is connected");
    check(backend.wireViews().size() == 2,
          "backend wires are visible to the frontend");
    check(backend.validate(), "integrated circuit passes DRC");

    backend.recordHistory();
    std::vector<ComponentInstance> frontend = backend.frontendComponents();
    for (ComponentInstance& component : frontend)
    {
        if (component.id() == resistor)
        {
            component.setPosition({180.0, 40.0});
            check(backend.syncComponent(component),
                  "frontend movement is synchronized to backend");
        }
    }
    backend.recordHistory();
    check(backend.undo(), "undo restores the previous integrated snapshot");

    bool restoredPosition = false;
    for (const ComponentInstance& component : backend.frontendComponents())
    {
        if (component.label() == "R1")
        {
            restoredPosition = component.position().x == 100.0 &&
                               component.position().y == 0.0;
        }
    }
    check(restoredPosition, "undo restores frontend-visible component state");

    const char* projectPath = "integration_test_project.txt";
    check(backend.save(projectPath), "project saves through FileManager");

    BackendAdapter loaded;
    check(loaded.load(projectPath), "saved project loads through FileManager");
    check(loaded.frontendComponents().size() == 3,
          "loaded backend rebuilds the frontend component list");
    check(loaded.wireViews().size() == 2,
          "loaded project restores wires");

    BackendAdapter catalog;
    const std::vector<std::string> defaultValues = {
        "1K", "100N", "9V", "1HZ", "OPEN", "2 INPUTS", "RED", "0V",
        "1M", "5V", "RELEASED", "0", "2 INPUTS", "INVERTER",
        "2 INPUTS", "2 INPUTS", "D TYPE", "4 BIT", "4 BIT", "1US",
        "8 BIT", "16X2", "4X4", "AUTO", "AUTO"
    };
    bool completeCatalog = true;
    for (int definitionId = 0;
         definitionId < static_cast<int>(defaultValues.size());
         ++definitionId)
    {
        completeCatalog = catalog.addComponent(
                definitionId,
                "X" + std::to_string(definitionId),
                defaultValues[definitionId],
                {static_cast<double>(definitionId * 20), 200.0}) >= 0 &&
                completeCatalog;
    }
    check(completeCatalog && catalog.frontendComponents().size() == 25,
          "all integrated component definitions create backend objects");

    const char* catalogPath = "integration_catalog_project.txt";
    check(catalog.save(catalogPath), "complete component catalog saves");
    BackendAdapter loadedCatalog;
    check(loadedCatalog.load(catalogPath) &&
          loadedCatalog.frontendComponents().size() == 25,
          "complete component catalog loads through the integration layer");

    std::remove(projectPath);
    std::remove(catalogPath);
    std::remove("recent_projects.txt");

    std::cout << "Integration failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
