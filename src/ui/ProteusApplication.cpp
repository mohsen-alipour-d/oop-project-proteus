#include "ProteusApplication.h"

#include <SDL2/SDL2_gfxPrimitives.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

Button::Button(int x,
               int y,
               int width,
               int height,
               const std::string& text,
               ButtonType type,
               bool isClickable)
        : buttonBounds{x, y, width, height},
          buttonText(text),
          buttonType(type),
          clickable(isClickable)
{
}

bool Button::contains(int mouseX, int mouseY) const
{
    return clickable &&
           mouseX >= buttonBounds.x &&
           mouseX < buttonBounds.x + buttonBounds.w &&
           mouseY >= buttonBounds.y &&
           mouseY < buttonBounds.y + buttonBounds.h;
}

void Button::updateHover(int mouseX, int mouseY)
{
    hovered = contains(mouseX, mouseY);
}

const SDL_Rect& Button::bounds() const
{
    return buttonBounds;
}

const std::string& Button::text() const
{
    return buttonText;
}

ButtonType Button::type() const
{
    return buttonType;
}

bool Button::isHovered() const
{
    return hovered;
}

ProteusApplication::~ProteusApplication()
{
    cleanUp();
}

bool ProteusApplication::initialize()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return false;
    }

    window = SDL_CreateWindow(
            "Proteus OOP Simulator - Integrated Sections 1 to 11",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            LOGICAL_WIDTH,
            LOGICAL_HEIGHT,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        return false;
    }

    renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC |
            SDL_RENDERER_TARGETTEXTURE
    );

    if (renderer == nullptr)
    {
        renderer = SDL_CreateRenderer(
                window,
                -1,
                SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE
        );
    }

    if (renderer == nullptr)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderSetLogicalSize(renderer, LOGICAL_WIDTH, LOGICAL_HEIGHT);

    activeDefinitionIds = {0, 1, 2, 6};
    previewDefinitionId = 0;
    lastFrameTicks = SDL_GetTicks64();
    showMainMenu();
    return true;
}

void ProteusApplication::run()
{
    while (running)
    {
        processEvents();
        updateSimulation();
        render();
    }
}

SDL_Rect ProteusApplication::toolbarArea() const
{
    return {0, 0, LOGICAL_WIDTH, TOOLBAR_HEIGHT};
}

SDL_Rect ProteusApplication::leftPanelArea() const
{
    return {0,
            TOOLBAR_HEIGHT,
            LEFT_PANEL_WIDTH,
            LOGICAL_HEIGHT - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT};
}

SDL_Rect ProteusApplication::rightPanelArea() const
{
    return {LOGICAL_WIDTH - RIGHT_PANEL_WIDTH,
            TOOLBAR_HEIGHT,
            RIGHT_PANEL_WIDTH,
            LOGICAL_HEIGHT - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT};
}

SDL_Rect ProteusApplication::canvasArea() const
{
    return {LEFT_PANEL_WIDTH,
            TOOLBAR_HEIGHT,
            LOGICAL_WIDTH - LEFT_PANEL_WIDTH - RIGHT_PANEL_WIDTH,
            LOGICAL_HEIGHT - TOOLBAR_HEIGHT - STATUS_BAR_HEIGHT};
}

SDL_Rect ProteusApplication::statusBarArea() const
{
    return {0,
            LOGICAL_HEIGHT - STATUS_BAR_HEIGHT,
            LOGICAL_WIDTH,
            STATUS_BAR_HEIGHT};
}

bool ProteusApplication::pointInsideRectangle(int x,
                                              int y,
                                              const SDL_Rect& rectangle)
{
    return x >= rectangle.x && x < rectangle.x + rectangle.w &&
           y >= rectangle.y && y < rectangle.y + rectangle.h;
}

SDL_Rect ProteusApplication::normalizedRectangle(const SDL_Point& first,
                                                 const SDL_Point& second)
{
    const int left = std::min(first.x, second.x);
    const int top = std::min(first.y, second.y);
    const int right = std::max(first.x, second.x);
    const int bottom = std::max(first.y, second.y);
    return {left, top, right - left, bottom - top};
}

void ProteusApplication::showMainMenu()
{
    currentPage = Page::MainMenu;
    selectedPreset = CanvasPreset::None;
    menuButtons.clear();
    searchFocused = false;
    customSizeDialog.open = false;
    SDL_StopTextInput();

    menuButtons.emplace_back(350, 18, 660, 112,
                             "PROTEUS SIMULATOR", ButtonType::PageTitle, false);
    menuButtons.emplace_back(455, 166, 450, 104,
                             "NEW PROJECT", ButtonType::NewProject);
    menuButtons.emplace_back(455, 292, 450, 104,
                             "OPEN EXISTING PROJECT", ButtonType::OpenProject);
    menuButtons.emplace_back(455, 418, 450, 104,
                             "RECENT PROJECTS", ButtonType::RecentProjects);
    menuButtons.emplace_back(455, 544, 450, 104,
                             "EXIT", ButtonType::Exit);
}

void ProteusApplication::showNewProjectMenu()
{
    currentPage = Page::NewProjectMenu;
    selectedPreset = CanvasPreset::None;
    menuButtons.clear();
    customSizeDialog.open = false;
    SDL_StopTextInput();

    menuButtons.emplace_back(350, 35, 660, 112,
                             "NEW PROJECT", ButtonType::PageTitle, false);
    menuButtons.emplace_back(455, 196, 450, 104,
                             "A4 - 297 X 210 MM", ButtonType::A4);
    menuButtons.emplace_back(455, 326, 450, 104,
                             "A3 - 420 X 297 MM", ButtonType::A3);
    menuButtons.emplace_back(455, 456, 450, 104,
                             "CUSTOM SIZE", ButtonType::Customize);
}

void ProteusApplication::showDesignWorkspace(CanvasPreset preset)
{
    currentPage = Page::DesignWorkspace;
    selectedPreset = preset;
    menuButtons.clear();
    customSizeDialog.open = false;
    propertiesDialog.open = false;
    searchFocused = false;
    contextMenuOpen = false;
    placementDefinitionId = -1;
    mouseOperation = MouseOperation::None;
    SDL_StopTextInput();
    resetView();

    std::cout << presetName() << " design workspace opened.\n";
}

void ProteusApplication::startNewProject(CanvasPreset preset)
{
    backend.resetProject();
    components.clear();
    selectedComponentIds.clear();
    pendingWireStart.reset();
    selectedWireId = -1;
    nextComponentId = 0;
    nextLabelNumber.clear();
    simulationRunning = false;
    statusMessage = "NEW PROJECT";
    showDesignWorkspace(preset);
}

bool ProteusApplication::loadProject(bool mostRecent)
{
    const bool loaded = mostRecent
                        ? backend.loadMostRecent()
                        : backend.load("proteus_project.txt");
    if (!loaded)
    {
        statusMessage = mostRecent ? "NO RECENT PROJECT" : "OPEN FAILED";
        std::cout << statusMessage << '\n';
        return false;
    }

    reloadFrontendFromBackend();
    simulationRunning = false;
    statusMessage = "PROJECT LOADED";
    showDesignWorkspace(CanvasPreset::A4);
    return true;
}

void ProteusApplication::reloadFrontendFromBackend()
{
    components = backend.frontendComponents();
    selectedComponentIds.clear();
    pendingWireStart.reset();
    selectedWireId = -1;
    nextComponentId = 0;
    for (const ComponentInstance& component : components)
    {
        nextComponentId = std::max(nextComponentId, component.id() + 1);
    }
    rebuildLabelNumbers();
}

void ProteusApplication::rebuildLabelNumbers()
{
    nextLabelNumber.clear();
    for (const ComponentInstance& component : components)
    {
        const ComponentDefinition* definition = componentLibrary.findById(
                component.definitionId());
        if (definition == nullptr ||
            component.label().rfind(definition->prefix, 0) != 0)
        {
            continue;
        }
        try
        {
            const int number = std::stoi(component.label().substr(
                    definition->prefix.size()));
            nextLabelNumber[component.definitionId()] = std::max(
                    nextLabelNumber[component.definitionId()], number);
        }
        catch (const std::exception&)
        {
            // A custom label is valid; it simply does not affect auto-numbering.
        }
    }
}

void ProteusApplication::updateSimulation()
{
    const Uint64 now = SDL_GetTicks64();
    const double dt = std::min(0.05,
                               static_cast<double>(now - lastFrameTicks) / 1000.0);
    lastFrameTicks = now;
    if (simulationRunning)
    {
        backend.step(dt);
    }
}

void ProteusApplication::processEvents()
{
    SDL_Event event{};

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_MOUSEMOTION:
            {
                const SDL_Point mouse = logicalMousePosition(event.motion.x,
                                                              event.motion.y);
                handleMouseMotion(mouse.x, mouse.y);
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
            {
                const SDL_Point mouse = logicalMousePosition(event.button.x,
                                                              event.button.y);
                handleMouseButtonDown(event.button, mouse.x, mouse.y);
                break;
            }

            case SDL_MOUSEBUTTONUP:
            {
                const SDL_Point mouse = logicalMousePosition(event.button.x,
                                                              event.button.y);
                handleMouseButtonUp(event.button, mouse.x, mouse.y);
                break;
            }

            case SDL_MOUSEWHEEL:
                handleMouseWheel(event.wheel);
                break;

            case SDL_KEYDOWN:
                handleKeyDown(event.key);
                break;

            case SDL_KEYUP:
                handleKeyUp(event.key);
                break;

            case SDL_TEXTINPUT:
                handleTextInput(event.text.text);
                break;

            default:
                break;
        }
    }
}

SDL_Point ProteusApplication::logicalMousePosition(int windowX, int windowY) const
{
    float logicalX = static_cast<float>(windowX);
    float logicalY = static_cast<float>(windowY);
    SDL_RenderWindowToLogical(renderer, windowX, windowY, &logicalX, &logicalY);
    return {static_cast<int>(std::lround(logicalX)),
            static_cast<int>(std::lround(logicalY))};
}

void ProteusApplication::handleMouseMotion(int mouseX, int mouseY)
{
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    if (currentPage != Page::DesignWorkspace)
    {
        for (Button& button : menuButtons)
        {
            button.updateHover(mouseX, mouseY);
        }
        return;
    }

    const SDL_Rect canvas = canvasArea();
    mouseInsideCanvas = pointInsideRectangle(mouseX, mouseY, canvas);
    if (mouseInsideCanvas)
    {
        canvasMouseWorld = screenToWorld(mouseX, mouseY);
    }

    switch (mouseOperation)
    {
        case MouseOperation::DragComponents:
            updateComponentDrag(screenToWorld(mouseX, mouseY));
            break;

        case MouseOperation::BoxSelect:
            selectionCurrentScreen = {mouseX, mouseY};
            break;

        case MouseOperation::Pan:
            panX = panStartX + static_cast<double>(mouseX - panStartScreen.x);
            panY = panStartY + static_cast<double>(mouseY - panStartScreen.y);
            break;

        case MouseOperation::None:
            break;
    }
}

void ProteusApplication::handleMouseButtonDown(const SDL_MouseButtonEvent& event,
                                                int mouseX,
                                                int mouseY)
{
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    if (customSizeDialog.open)
    {
        if (event.button == SDL_BUTTON_LEFT)
        {
            handleCustomDialogClick(mouseX, mouseY);
        }
        return;
    }

    if (propertiesDialog.open)
    {
        if (event.button == SDL_BUTTON_LEFT)
        {
            handlePropertiesDialogClick(mouseX, mouseY);
        }
        return;
    }

    if (currentPage != Page::DesignWorkspace)
    {
        if (event.button == SDL_BUTTON_LEFT)
        {
            handleMenuClick(mouseX, mouseY);
        }
        return;
    }

    if (event.button == SDL_BUTTON_MIDDLE ||
        (event.button == SDL_BUTTON_LEFT && spaceHeld))
    {
        if (pointInsideRectangle(mouseX, mouseY, canvasArea()))
        {
            beginPan(mouseX, mouseY);
        }
        return;
    }

    if (event.button == SDL_BUTTON_RIGHT)
    {
        handleWorkspaceRightClick(mouseX, mouseY);
        return;
    }

    if (event.button == SDL_BUTTON_LEFT)
    {
        handleWorkspaceLeftClick(mouseX,
                                 mouseY,
                                 event.clicks,
                                 SDL_GetModState());
    }
}

void ProteusApplication::handleMouseButtonUp(const SDL_MouseButtonEvent& event,
                                              int mouseX,
                                              int mouseY)
{
    if (currentPage != Page::DesignWorkspace)
    {
        return;
    }

    if (event.button == SDL_BUTTON_LEFT || event.button == SDL_BUTTON_MIDDLE)
    {
        finishCanvasOperation(mouseX, mouseY, SDL_GetModState());
    }
}

void ProteusApplication::handleMouseWheel(const SDL_MouseWheelEvent& event)
{
    if (currentPage != Page::DesignWorkspace || propertiesDialog.open ||
        customSizeDialog.open)
    {
        return;
    }

    if (pointInsideRectangle(lastMouseX, lastMouseY, canvasArea()))
    {
        const double factor = event.y > 0 ? 1.12 : 1.0 / 1.12;
        zoomAt(lastMouseX, lastMouseY, factor);
        return;
    }

    if (pointInsideRectangle(lastMouseX, lastMouseY, leftPanelArea()))
    {
        const int direction = event.y > 0 ? -1 : 1;
        if (lastMouseY < leftPanelArea().y + 515)
        {
            const int filteredSize = static_cast<int>(
                    componentLibrary.filteredIds(selectedCategory, searchText).size());
            const int maximumLibraryOffset = std::max(
                    0, filteredSize - MAX_LIBRARY_ROWS);
            libraryScrollOffset = std::clamp(libraryScrollOffset + direction,
                                             0,
                                             maximumLibraryOffset);
        }
        else
        {
            const int maximumActiveOffset = std::max(
                    0, static_cast<int>(activeDefinitionIds.size()) -
                       MAX_ACTIVE_ROWS);
            activeScrollOffset = std::clamp(activeScrollOffset + direction,
                                            0,
                                            maximumActiveOffset);
        }
    }
}

void ProteusApplication::handleKeyDown(const SDL_KeyboardEvent& event)
{
    const SDL_Keycode key = event.keysym.sym;
    const SDL_Keymod modifiers = static_cast<SDL_Keymod>(event.keysym.mod);

    if (key == SDLK_SPACE)
    {
        spaceHeld = true;
    }

    if (customSizeDialog.open)
    {
        if (key == SDLK_ESCAPE)
        {
            closeCustomSizeDialog(false);
        }
        else if (key == SDLK_TAB)
        {
            customSizeDialog.activeField = 1 - customSizeDialog.activeField;
        }
        else if (key == SDLK_BACKSPACE)
        {
            removeLastUtf8Character(activeDialogText());
        }
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
        {
            closeCustomSizeDialog(true);
        }
        return;
    }

    if (propertiesDialog.open)
    {
        if (key == SDLK_ESCAPE)
        {
            closePropertiesDialog(false);
        }
        else if (key == SDLK_TAB)
        {
            propertiesDialog.activeField = 1 - propertiesDialog.activeField;
        }
        else if (key == SDLK_BACKSPACE)
        {
            removeLastUtf8Character(activeDialogText());
        }
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
        {
            closePropertiesDialog(true);
        }
        return;
    }

    if (searchFocused)
    {
        if (key == SDLK_BACKSPACE)
        {
            removeLastUtf8Character(searchText);
            libraryScrollOffset = 0;
        }
        else if (key == SDLK_ESCAPE || key == SDLK_RETURN ||
                 key == SDLK_KP_ENTER)
        {
            searchFocused = false;
            SDL_StopTextInput();
        }
        return;
    }

    if (currentPage != Page::DesignWorkspace)
    {
        if (key == SDLK_ESCAPE)
        {
            if (currentPage == Page::NewProjectMenu)
            {
                showMainMenu();
            }
            else
            {
                running = false;
            }
        }
        return;
    }

    if (key == SDLK_ESCAPE)
    {
        if (pendingWireStart.has_value())
        {
            pendingWireStart.reset();
            statusMessage = "WIRE CANCELLED";
        }
        else if (contextMenuOpen)
        {
            contextMenuOpen = false;
        }
        else if (placementDefinitionId >= 0)
        {
            placementDefinitionId = -1;
        }
        else if (mouseOperation != MouseOperation::None)
        {
            mouseOperation = MouseOperation::None;
        }
        else
        {
            showNewProjectMenu();
        }
        return;
    }

    if ((modifiers & KMOD_CTRL) != 0 && key == SDLK_s)
    {
        saveProject();
    }
    else if ((modifiers & KMOD_CTRL) != 0 && key == SDLK_z)
    {
        undoProject();
    }
    else if ((modifiers & KMOD_CTRL) != 0 && key == SDLK_y)
    {
        redoProject();
    }
    else if ((modifiers & KMOD_CTRL) != 0 && key == SDLK_a)
    {
        selectAllComponents();
    }
    else if (key == SDLK_DELETE || key == SDLK_BACKSPACE)
    {
        deleteSelectedComponents();
    }
    else if (key == SDLK_r)
    {
        rotateSelectedComponents();
    }
    else if (key == SDLK_h)
    {
        mirrorSelectedComponentsHorizontally();
    }
    else if (key == SDLK_v)
    {
        mirrorSelectedComponentsVertically();
    }
    else if (key == SDLK_j && mouseInsideCanvas)
    {
        if (backend.addJunctionAt(canvasMouseWorld))
        {
            recordProjectChange();
            statusMessage = "JUNCTION ADDED";
        }
        else
        {
            statusMessage = "JUNCTION NEEDS 2 WIRES";
        }
    }
}

void ProteusApplication::handleKeyUp(const SDL_KeyboardEvent& event)
{
    if (event.keysym.sym == SDLK_SPACE)
    {
        spaceHeld = false;
    }
}

void ProteusApplication::handleTextInput(const std::string& text)
{
    if (customSizeDialog.open || propertiesDialog.open)
    {
        std::string& target = activeDialogText();
        if (target.size() + text.size() <= 28)
        {
            target += text;
        }
        return;
    }

    if (searchFocused && searchText.size() + text.size() <= 24)
    {
        searchText += toUpperAscii(text);
        libraryScrollOffset = 0;
    }
}

void ProteusApplication::handleMenuClick(int mouseX, int mouseY)
{
    for (const Button& button : menuButtons)
    {
        if (button.contains(mouseX, mouseY))
        {
            performMenuAction(button.type());
            return;
        }
    }
}

void ProteusApplication::performMenuAction(ButtonType type)
{
    switch (type)
    {
        case ButtonType::NewProject:
            showNewProjectMenu();
            break;

        case ButtonType::A4:
            startNewProject(CanvasPreset::A4);
            break;

        case ButtonType::A3:
            startNewProject(CanvasPreset::A3);
            break;

        case ButtonType::Customize:
            openCustomSizeDialog();
            break;

        case ButtonType::OpenProject:
            loadProject(false);
            break;

        case ButtonType::RecentProjects:
            loadProject(true);
            break;

        case ButtonType::Exit:
            running = false;
            break;

        case ButtonType::PageTitle:
            break;
    }
}

void ProteusApplication::handleWorkspaceLeftClick(int mouseX,
                                                  int mouseY,
                                                  int clickCount,
                                                  SDL_Keymod modifiers)
{
    if (contextMenuOpen)
    {
        if (handleContextMenuClick(mouseX, mouseY))
        {
            return;
        }
        contextMenuOpen = false;
    }

    if (handleToolbarClick(mouseX, mouseY) ||
        handleLeftPanelClick(mouseX, mouseY, clickCount) ||
        handleRightPanelClick(mouseX, mouseY))
    {
        return;
    }

    if (!pointInsideRectangle(mouseX, mouseY, canvasArea()))
    {
        searchFocused = false;
        SDL_StopTextInput();
        return;
    }

    searchFocused = false;
    SDL_StopTextInput();
    const WorldPoint worldPoint = screenToWorld(mouseX, mouseY);

    if (placementDefinitionId >= 0)
    {
        placeComponent(placementDefinitionId, snapPoint(worldPoint));
        return;
    }

    const std::optional<PinHandle> hitPin = backend.findPinAt(
            worldPoint, 8.0 / zoom);
    if (hitPin.has_value())
    {
        selectedWireId = -1;
        clearSelection();
        if (!pendingWireStart.has_value())
        {
            pendingWireStart = hitPin;
            statusMessage = "SELECT SECOND PIN";
        }
        else if (*pendingWireStart == *hitPin)
        {
            pendingWireStart.reset();
            statusMessage = "WIRE CANCELLED";
        }
        else
        {
            if (backend.connectPins(*pendingWireStart, *hitPin))
            {
                recordProjectChange();
                statusMessage = "WIRE CONNECTED";
            }
            else
            {
                statusMessage = "WIRE NOT CREATED";
            }
            pendingWireStart.reset();
        }
        return;
    }

    const int hitComponentId = componentAt(worldPoint);

    if (hitComponentId >= 0 && clickCount >= 2)
    {
        selectOnly(hitComponentId);
        openPropertiesDialog(hitComponentId);
        return;
    }

    if (hitComponentId >= 0)
    {
        if ((modifiers & KMOD_SHIFT) != 0 || (modifiers & KMOD_CTRL) != 0)
        {
            toggleSelection(hitComponentId);
            if (selectedComponentIds.count(hitComponentId) == 0)
            {
                return;
            }
        }
        else if (selectedComponentIds.count(hitComponentId) == 0)
        {
            selectOnly(hitComponentId);
        }

        startComponentDrag(hitComponentId, worldPoint);
        return;
    }

    selectedWireId = backend.findWireAt(worldPoint, 6.0 / zoom);
    if (selectedWireId >= 0)
    {
        clearSelection();
        statusMessage = "WIRE SELECTED";
        return;
    }

    if ((modifiers & KMOD_SHIFT) == 0 && (modifiers & KMOD_CTRL) == 0)
    {
        clearSelection();
    }
    beginBoxSelection(mouseX, mouseY);
}

void ProteusApplication::handleWorkspaceRightClick(int mouseX, int mouseY)
{
    if (!pointInsideRectangle(mouseX, mouseY, canvasArea()))
    {
        contextMenuOpen = false;
        return;
    }

    placementDefinitionId = -1;
    const int hitComponentId = componentAt(screenToWorld(mouseX, mouseY));
    if (hitComponentId < 0)
    {
        contextMenuOpen = false;
        return;
    }

    if (selectedComponentIds.count(hitComponentId) == 0)
    {
        selectOnly(hitComponentId);
    }

    contextMenuOpen = true;
    contextMenuPosition = {
        std::clamp(mouseX, 4, LOGICAL_WIDTH - 178),
        std::clamp(mouseY, 4, LOGICAL_HEIGHT - 154)
    };
}

void ProteusApplication::finishCanvasOperation(int mouseX,
                                               int mouseY,
                                               SDL_Keymod modifiers)
{
    if (mouseOperation == MouseOperation::DragComponents)
    {
        for (int componentId : selectedComponentIds)
        {
            ComponentInstance* component = componentById(componentId);
            if (component != nullptr)
            {
                component->setPosition(snapPoint(component->position()));
                backend.syncComponent(*component);
            }
        }
        recordProjectChange();
    }
    else if (mouseOperation == MouseOperation::BoxSelect)
    {
        selectionCurrentScreen = {mouseX, mouseY};
        applyBoxSelection(modifiers);
    }

    mouseOperation = MouseOperation::None;
    originalDragPositions.clear();
}

bool ProteusApplication::handleLeftPanelClick(int mouseX,
                                              int mouseY,
                                              int clickCount)
{
    if (!pointInsideRectangle(mouseX, mouseY, leftPanelArea()))
    {
        return false;
    }

    rebuildLeftPanelHitAreas();

    if (pointInsideRectangle(mouseX, mouseY, searchBox))
    {
        searchFocused = true;
        SDL_StartTextInput();
        return true;
    }

    searchFocused = false;
    SDL_StopTextInput();

    for (const auto& categoryRow : categoryRows)
    {
        if (pointInsideRectangle(mouseX, mouseY, categoryRow.first))
        {
            selectedCategory = categoryRow.second;
            libraryScrollOffset = 0;
            const std::vector<int> filtered = componentLibrary.filteredIds(
                    selectedCategory, searchText);
            if (!filtered.empty())
            {
                previewDefinitionId = filtered.front();
            }
            return true;
        }
    }

    for (const ComponentRow& row : libraryRows)
    {
        if (pointInsideRectangle(mouseX, mouseY, row.bounds))
        {
            previewDefinitionId = row.definitionId;
            if (clickCount >= 2)
            {
                addDefinitionToActiveList(row.definitionId);
                armComponentForPlacement(row.definitionId);
            }
            return true;
        }
    }

    if (pointInsideRectangle(mouseX, mouseY, addToActiveButton))
    {
        addDefinitionToActiveList(previewDefinitionId);
        return true;
    }

    for (const ComponentRow& row : activeRows)
    {
        if (pointInsideRectangle(mouseX, mouseY, row.removeBounds))
        {
            removeDefinitionFromActiveList(row.definitionId);
            return true;
        }

        if (pointInsideRectangle(mouseX, mouseY, row.bounds))
        {
            previewDefinitionId = row.definitionId;
            armComponentForPlacement(row.definitionId);
            return true;
        }
    }

    return true;
}

bool ProteusApplication::handleToolbarClick(int mouseX, int mouseY)
{
    if (!pointInsideRectangle(mouseX, mouseY, toolbarArea()))
    {
        return false;
    }

    rebuildToolbarHitAreas();
    for (const auto& button : toolbarButtons)
    {
        if (!pointInsideRectangle(mouseX, mouseY, button.first))
        {
            continue;
        }

        switch (button.second)
        {
            case ToolbarAction::Select:
                placementDefinitionId = -1;
                break;

            case ToolbarAction::Rotate:
                rotateSelectedComponents();
                break;

            case ToolbarAction::MirrorHorizontal:
                mirrorSelectedComponentsHorizontally();
                break;

            case ToolbarAction::MirrorVertical:
                mirrorSelectedComponentsVertically();
                break;

            case ToolbarAction::Delete:
                deleteSelectedComponents();
                break;

            case ToolbarAction::Save:
                saveProject();
                break;

            case ToolbarAction::Undo:
                undoProject();
                break;

            case ToolbarAction::Redo:
                redoProject();
                break;

            case ToolbarAction::Drc:
                runDesignRuleCheck();
                break;

            case ToolbarAction::RunPause:
                toggleSimulation();
                break;

            case ToolbarAction::Stop:
                stopSimulation();
                break;

            case ToolbarAction::ResetView:
                resetView();
                break;
        }
        return true;
    }

    return true;
}

bool ProteusApplication::handleRightPanelClick(int mouseX, int mouseY)
{
    if (!pointInsideRectangle(mouseX, mouseY, rightPanelArea()))
    {
        return false;
    }

    if (pointInsideRectangle(mouseX, mouseY, editPropertiesButton) &&
        selectedComponentIds.size() == 1)
    {
        openPropertiesDialog(*selectedComponentIds.begin());
    }

    return true;
}

bool ProteusApplication::handleContextMenuClick(int mouseX, int mouseY)
{
    constexpr int itemHeight = 30;
    const SDL_Rect menu{contextMenuPosition.x,
                        contextMenuPosition.y,
                        174,
                        itemHeight * 5};

    if (!pointInsideRectangle(mouseX, mouseY, menu))
    {
        return false;
    }

    const int selectedItem = (mouseY - menu.y) / itemHeight;
    contextMenuOpen = false;

    switch (selectedItem)
    {
        case 0:
            rotateSelectedComponents();
            break;

        case 1:
            mirrorSelectedComponentsHorizontally();
            break;

        case 2:
            mirrorSelectedComponentsVertically();
            break;

        case 3:
            if (selectedComponentIds.size() == 1)
            {
                openPropertiesDialog(*selectedComponentIds.begin());
            }
            break;

        case 4:
            deleteSelectedComponents();
            break;

        default:
            break;
    }

    return true;
}

bool ProteusApplication::handlePropertiesDialogClick(int mouseX, int mouseY)
{
    const SDL_Rect dialog{420, 205, 520, 390};
    const SDL_Rect labelField{dialog.x + 170, dialog.y + 94, 300, 42};
    const SDL_Rect valueField{dialog.x + 170, dialog.y + 158, 300, 42};
    const SDL_Rect saveButton{dialog.x + 275, dialog.y + 320, 95, 42};
    const SDL_Rect cancelButton{dialog.x + 380, dialog.y + 320, 95, 42};

    if (pointInsideRectangle(mouseX, mouseY, labelField))
    {
        propertiesDialog.activeField = 0;
        SDL_StartTextInput();
    }
    else if (pointInsideRectangle(mouseX, mouseY, valueField))
    {
        propertiesDialog.activeField = 1;
        SDL_StartTextInput();
    }
    else if (pointInsideRectangle(mouseX, mouseY, saveButton))
    {
        closePropertiesDialog(true);
    }
    else if (pointInsideRectangle(mouseX, mouseY, cancelButton))
    {
        closePropertiesDialog(false);
    }

    return true;
}

bool ProteusApplication::handleCustomDialogClick(int mouseX, int mouseY)
{
    const SDL_Rect dialog{430, 215, 500, 340};
    const SDL_Rect widthField{dialog.x + 175, dialog.y + 92, 270, 42};
    const SDL_Rect heightField{dialog.x + 175, dialog.y + 154, 270, 42};
    const SDL_Rect createButton{dialog.x + 235, dialog.y + 270, 100, 42};
    const SDL_Rect cancelButton{dialog.x + 345, dialog.y + 270, 100, 42};

    if (pointInsideRectangle(mouseX, mouseY, widthField))
    {
        customSizeDialog.activeField = 0;
        SDL_StartTextInput();
    }
    else if (pointInsideRectangle(mouseX, mouseY, heightField))
    {
        customSizeDialog.activeField = 1;
        SDL_StartTextInput();
    }
    else if (pointInsideRectangle(mouseX, mouseY, createButton))
    {
        closeCustomSizeDialog(true);
    }
    else if (pointInsideRectangle(mouseX, mouseY, cancelButton))
    {
        closeCustomSizeDialog(false);
    }

    return true;
}

void ProteusApplication::startComponentDrag(int componentId,
                                            const WorldPoint& mouseWorld)
{
    if (selectedComponentIds.count(componentId) == 0)
    {
        return;
    }

    mouseOperation = MouseOperation::DragComponents;
    dragStartWorld = mouseWorld;
    originalDragPositions.clear();

    for (int selectedId : selectedComponentIds)
    {
        const ComponentInstance* component = componentById(selectedId);
        if (component != nullptr)
        {
            originalDragPositions[selectedId] = component->position();
        }
    }
}

void ProteusApplication::beginBoxSelection(int mouseX, int mouseY)
{
    mouseOperation = MouseOperation::BoxSelect;
    selectionStartScreen = {mouseX, mouseY};
    selectionCurrentScreen = selectionStartScreen;
}

void ProteusApplication::beginPan(int mouseX, int mouseY)
{
    contextMenuOpen = false;
    mouseOperation = MouseOperation::Pan;
    panStartScreen = {mouseX, mouseY};
    panStartX = panX;
    panStartY = panY;
}

void ProteusApplication::updateComponentDrag(const WorldPoint& mouseWorld)
{
    const double deltaX = mouseWorld.x - dragStartWorld.x;
    const double deltaY = mouseWorld.y - dragStartWorld.y;

    for (const auto& original : originalDragPositions)
    {
        ComponentInstance* component = componentById(original.first);
        if (component == nullptr)
        {
            continue;
        }

        component->setPosition(snapPoint({original.second.x + deltaX,
                                          original.second.y + deltaY}));
        backend.syncComponent(*component);
    }
}

void ProteusApplication::applyBoxSelection(SDL_Keymod modifiers)
{
    const SDL_Rect selection = normalizedRectangle(selectionStartScreen,
                                                   selectionCurrentScreen);
    if (selection.w < 3 && selection.h < 3)
    {
        return;
    }

    const WorldPoint first = screenToWorld(selection.x, selection.y);
    const WorldPoint second = screenToWorld(selection.x + selection.w,
                                            selection.y + selection.h);
    const WorldRect worldSelection{
        std::min(first.x, second.x),
        std::min(first.y, second.y),
        std::abs(second.x - first.x),
        std::abs(second.y - first.y)
    };

    if ((modifiers & KMOD_SHIFT) == 0 && (modifiers & KMOD_CTRL) == 0)
    {
        clearSelection();
    }

    for (const ComponentInstance& component : components)
    {
        const ComponentDefinition* definition = componentLibrary.findById(
                component.definitionId());
        if (definition != nullptr && worldSelection.intersects(component.bounds(*definition)))
        {
            selectedComponentIds.insert(component.id());
        }
    }
}

void ProteusApplication::armComponentForPlacement(int definitionId)
{
    if (componentLibrary.findById(definitionId) == nullptr)
    {
        return;
    }

    placementDefinitionId = definitionId;
    contextMenuOpen = false;
}

void ProteusApplication::placeComponent(int definitionId,
                                        const WorldPoint& worldPosition)
{
    const ComponentDefinition* definition = componentLibrary.findById(definitionId);
    if (definition == nullptr)
    {
        return;
    }

    std::string label;
    do
    {
        const int labelNumber = ++nextLabelNumber[definitionId];
        label = definition->prefix + std::to_string(labelNumber);
    }
    while (!backend.isLabelAvailable(label, -1));

    const int componentId = backend.addComponent(definitionId,
                                                  label,
                                                  definition->defaultValue,
                                                  worldPosition);
    if (componentId < 0)
    {
        statusMessage = "COMPONENT CREATE FAILED";
        return;
    }

    components.emplace_back(componentId,
                            definitionId,
                            label,
                            definition->defaultValue,
                            worldPosition);
    selectOnly(componentId);
    nextComponentId = std::max(nextComponentId, componentId + 1);
    recordProjectChange();
    statusMessage = "COMPONENT PLACED";
}

void ProteusApplication::addDefinitionToActiveList(int definitionId)
{
    if (componentLibrary.findById(definitionId) == nullptr ||
        isDefinitionActive(definitionId))
    {
        return;
    }

    activeDefinitionIds.push_back(definitionId);
    const int maximumOffset = std::max(
            0, static_cast<int>(activeDefinitionIds.size()) - MAX_ACTIVE_ROWS);
    activeScrollOffset = maximumOffset;
}

void ProteusApplication::removeDefinitionFromActiveList(int definitionId)
{
    activeDefinitionIds.erase(
            std::remove(activeDefinitionIds.begin(),
                        activeDefinitionIds.end(),
                        definitionId),
            activeDefinitionIds.end());

    if (placementDefinitionId == definitionId)
    {
        placementDefinitionId = -1;
    }

    const int maximumOffset = std::max(
            0, static_cast<int>(activeDefinitionIds.size()) - MAX_ACTIVE_ROWS);
    activeScrollOffset = std::min(activeScrollOffset, maximumOffset);
}

bool ProteusApplication::isDefinitionActive(int definitionId) const
{
    return std::find(activeDefinitionIds.begin(), activeDefinitionIds.end(),
                     definitionId) != activeDefinitionIds.end();
}

ComponentInstance* ProteusApplication::componentById(int id)
{
    const auto found = std::find_if(components.begin(), components.end(),
                                    [id](const ComponentInstance& component)
                                    {
                                        return component.id() == id;
                                    });
    return found == components.end() ? nullptr : &(*found);
}

const ComponentInstance* ProteusApplication::componentById(int id) const
{
    const auto found = std::find_if(components.begin(), components.end(),
                                    [id](const ComponentInstance& component)
                                    {
                                        return component.id() == id;
                                    });
    return found == components.end() ? nullptr : &(*found);
}

int ProteusApplication::componentAt(const WorldPoint& point) const
{
    for (auto component = components.rbegin(); component != components.rend();
         ++component)
    {
        const ComponentDefinition* definition = componentLibrary.findById(
                component->definitionId());
        if (definition != nullptr && component->bounds(*definition).contains(point))
        {
            return component->id();
        }
    }

    return -1;
}

void ProteusApplication::selectOnly(int componentId)
{
    selectedComponentIds.clear();
    selectedWireId = -1;
    if (componentById(componentId) != nullptr)
    {
        selectedComponentIds.insert(componentId);
    }
}

void ProteusApplication::toggleSelection(int componentId)
{
    if (selectedComponentIds.count(componentId) != 0)
    {
        selectedComponentIds.erase(componentId);
    }
    else if (componentById(componentId) != nullptr)
    {
        selectedComponentIds.insert(componentId);
    }
}

void ProteusApplication::clearSelection()
{
    selectedComponentIds.clear();
}

void ProteusApplication::selectAllComponents()
{
    selectedComponentIds.clear();
    for (const ComponentInstance& component : components)
    {
        selectedComponentIds.insert(component.id());
    }
}

void ProteusApplication::deleteSelectedComponents()
{
    if (selectedComponentIds.empty() && selectedWireId >= 0)
    {
        if (backend.removeWire(selectedWireId))
        {
            selectedWireId = -1;
            pendingWireStart.reset();
            recordProjectChange();
            statusMessage = "WIRE DELETED";
        }
        return;
    }

    if (selectedComponentIds.empty())
    {
        return;
    }

    for (int componentId : selectedComponentIds)
    {
        backend.removeComponent(componentId);
    }

    components.erase(
            std::remove_if(components.begin(), components.end(),
                           [this](const ComponentInstance& component)
                           {
                               return selectedComponentIds.count(component.id()) != 0;
                           }),
            components.end());

    selectedComponentIds.clear();
    selectedWireId = -1;
    pendingWireStart.reset();
    contextMenuOpen = false;
    recordProjectChange();
    statusMessage = "COMPONENTS DELETED";
}

void ProteusApplication::rotateSelectedComponents()
{
    bool changed = false;
    for (int selectedId : selectedComponentIds)
    {
        ComponentInstance* component = componentById(selectedId);
        if (component != nullptr)
        {
            component->rotateClockwise();
            backend.syncComponent(*component);
            changed = true;
        }
    }
    if (changed)
    {
        recordProjectChange();
    }
}

void ProteusApplication::mirrorSelectedComponentsHorizontally()
{
    bool changed = false;
    for (int selectedId : selectedComponentIds)
    {
        ComponentInstance* component = componentById(selectedId);
        if (component != nullptr)
        {
            component->mirrorHorizontally();
            backend.syncComponent(*component);
            changed = true;
        }
    }
    if (changed)
    {
        recordProjectChange();
    }
}

void ProteusApplication::mirrorSelectedComponentsVertically()
{
    bool changed = false;
    for (int selectedId : selectedComponentIds)
    {
        ComponentInstance* component = componentById(selectedId);
        if (component != nullptr)
        {
            component->mirrorVertically();
            backend.syncComponent(*component);
            changed = true;
        }
    }
    if (changed)
    {
        recordProjectChange();
    }
}

void ProteusApplication::openPropertiesDialog(int componentId)
{
    ComponentInstance* component = componentById(componentId);
    if (component == nullptr)
    {
        return;
    }

    propertiesDialog.open = true;
    propertiesDialog.componentId = componentId;
    propertiesDialog.activeField = 0;
    propertiesDialog.labelDraft = component->label();
    propertiesDialog.valueDraft = component->value();
    contextMenuOpen = false;
    SDL_StartTextInput();
}

void ProteusApplication::closePropertiesDialog(bool saveChanges)
{
    if (saveChanges)
    {
        ComponentInstance* component = componentById(propertiesDialog.componentId);
        if (component != nullptr)
        {
            const std::string oldLabel = component->label();
            const std::string oldValue = component->value();
            const std::string label = trimAscii(propertiesDialog.labelDraft);
            const std::string value = trimAscii(propertiesDialog.valueDraft);

            if (!label.empty())
            {
                component->setLabel(label);
            }
            if (!value.empty())
            {
                component->setValue(value);
            }

            if (!backend.syncComponent(*component))
            {
                component->setLabel(oldLabel);
                component->setValue(oldValue);
                statusMessage = "LABEL MUST BE UNIQUE / NO SPACES";
            }
            else
            {
                recordProjectChange();
                statusMessage = "PROPERTIES SAVED";
            }
        }
    }

    propertiesDialog = PropertiesDialog{};
    SDL_StopTextInput();
}

void ProteusApplication::saveProject()
{
    if (backend.save())
    {
        statusMessage = "SAVED: " + backend.currentPath();
    }
    else
    {
        statusMessage = "SAVE FAILED";
    }
    std::cout << statusMessage << '\n';
}

void ProteusApplication::undoProject()
{
    if (!backend.undo())
    {
        statusMessage = "NOTHING TO UNDO";
        return;
    }
    reloadFrontendFromBackend();
    statusMessage = "UNDO";
}

void ProteusApplication::redoProject()
{
    if (!backend.redo())
    {
        statusMessage = "NOTHING TO REDO";
        return;
    }
    reloadFrontendFromBackend();
    statusMessage = "REDO";
}

void ProteusApplication::runDesignRuleCheck()
{
    const bool valid = backend.validate();
    statusMessage = valid ? "DRC PASSED" : "DRC FAILED - SEE CONSOLE";
    for (const LogMessage& message : backend.validationMessages())
    {
        std::cout << (message.isError ? "[DRC ERROR] " : "[DRC] ")
                  << message.text << '\n';
    }
}

void ProteusApplication::toggleSimulation()
{
    if (simulationRunning)
    {
        simulationRunning = false;
        statusMessage = "SIMULATION PAUSED";
        return;
    }

    if (!backend.validate())
    {
        statusMessage = "RUN BLOCKED BY DRC";
        for (const LogMessage& message : backend.validationMessages())
        {
            std::cout << (message.isError ? "[DRC ERROR] " : "[DRC] ")
                      << message.text << '\n';
        }
        return;
    }

    simulationRunning = true;
    lastFrameTicks = SDL_GetTicks64();
    statusMessage = "SIMULATION RUNNING";
}

void ProteusApplication::stopSimulation()
{
    simulationRunning = false;
    backend.stopSimulation();
    statusMessage = "SIMULATION STOPPED";
}

void ProteusApplication::recordProjectChange()
{
    backend.recordHistory();
}

void ProteusApplication::openCustomSizeDialog()
{
    customSizeDialog.open = true;
    customSizeDialog.activeField = 0;
    customSizeDialog.widthDraft = std::to_string(customCanvasWidthMillimeters);
    customSizeDialog.heightDraft = std::to_string(customCanvasHeightMillimeters);
    customSizeDialog.validationMessage.clear();
    SDL_StartTextInput();
}

void ProteusApplication::closeCustomSizeDialog(bool createProject)
{
    if (!createProject)
    {
        customSizeDialog.open = false;
        customSizeDialog.validationMessage.clear();
        SDL_StopTextInput();
        return;
    }

    try
    {
        const std::string widthText = trimAscii(customSizeDialog.widthDraft);
        const std::string heightText = trimAscii(customSizeDialog.heightDraft);
        std::size_t widthCharacters = 0;
        std::size_t heightCharacters = 0;
        const int width = std::stoi(widthText, &widthCharacters);
        const int height = std::stoi(heightText, &heightCharacters);
        if (widthCharacters != widthText.size() ||
            heightCharacters != heightText.size())
        {
            throw std::invalid_argument("trailing characters");
        }
        if (width < 100 || width > 2000 || height < 100 || height > 2000)
        {
            customSizeDialog.validationMessage = "USE VALUES FROM 100 TO 2000 MM";
            return;
        }

        customCanvasWidthMillimeters = width;
        customCanvasHeightMillimeters = height;
        customSizeDialog.open = false;
        SDL_StopTextInput();
        startNewProject(CanvasPreset::Custom);
    }
    catch (const std::exception&)
    {
        customSizeDialog.validationMessage = "WIDTH AND HEIGHT MUST BE NUMBERS";
    }
}

std::string& ProteusApplication::activeDialogText()
{
    if (customSizeDialog.open)
    {
        return customSizeDialog.activeField == 0
               ? customSizeDialog.widthDraft
               : customSizeDialog.heightDraft;
    }

    return propertiesDialog.activeField == 0
           ? propertiesDialog.labelDraft
           : propertiesDialog.valueDraft;
}

void ProteusApplication::removeLastUtf8Character(std::string& text)
{
    if (text.empty())
    {
        return;
    }

    std::size_t characterStart = text.size() - 1;
    while (characterStart > 0 &&
           (static_cast<unsigned char>(text[characterStart]) & 0xC0U) == 0x80U)
    {
        --characterStart;
    }
    text.erase(characterStart);
}

WorldPoint ProteusApplication::screenToWorld(int screenX, int screenY) const
{
    const SDL_Rect canvas = canvasArea();
    return {(static_cast<double>(screenX - canvas.x) - panX) / zoom,
            (static_cast<double>(screenY - canvas.y) - panY) / zoom};
}

SDL_Point ProteusApplication::worldToScreen(const WorldPoint& point) const
{
    const SDL_Rect canvas = canvasArea();
    return {canvas.x + static_cast<int>(std::lround(panX + point.x * zoom)),
            canvas.y + static_cast<int>(std::lround(panY + point.y * zoom))};
}

double ProteusApplication::snapCoordinate(double coordinate) const
{
    return std::round(coordinate / GRID_SPACING) * GRID_SPACING;
}

WorldPoint ProteusApplication::snapPoint(const WorldPoint& point) const
{
    return {snapCoordinate(point.x), snapCoordinate(point.y)};
}

SDL_Rect ProteusApplication::componentScreenBounds(
        const ComponentInstance& component,
        const ComponentDefinition& definition) const
{
    const WorldRect worldBounds = component.bounds(definition);
    const SDL_Point topLeft = worldToScreen({worldBounds.x, worldBounds.y});
    const SDL_Point bottomRight = worldToScreen({worldBounds.x + worldBounds.w,
                                                 worldBounds.y + worldBounds.h});
    return {std::min(topLeft.x, bottomRight.x),
            std::min(topLeft.y, bottomRight.y),
            std::abs(bottomRight.x - topLeft.x),
            std::abs(bottomRight.y - topLeft.y)};
}

void ProteusApplication::zoomAt(int screenX, int screenY, double factor)
{
    const WorldPoint pointBeforeZoom = screenToWorld(screenX, screenY);
    zoom = std::clamp(zoom * factor, 0.5, 2.5);

    const SDL_Rect canvas = canvasArea();
    panX = static_cast<double>(screenX - canvas.x) - pointBeforeZoom.x * zoom;
    panY = static_cast<double>(screenY - canvas.y) - pointBeforeZoom.y * zoom;
}

void ProteusApplication::resetView()
{
    zoom = 1.0;
    panX = 32.0;
    panY = 32.0;
}

std::string ProteusApplication::presetName() const
{
    switch (selectedPreset)
    {
        case CanvasPreset::A4:
            return "A4";
        case CanvasPreset::A3:
            return "A3";
        case CanvasPreset::Custom:
            return "CUSTOM";
        case CanvasPreset::None:
            return "NONE";
    }
    return "NONE";
}

std::string ProteusApplication::presetDimensions() const
{
    switch (selectedPreset)
    {
        case CanvasPreset::A4:
            return "297 X 210 MM";
        case CanvasPreset::A3:
            return "420 X 297 MM";
        case CanvasPreset::Custom:
            return std::to_string(customCanvasWidthMillimeters) + " X " +
                   std::to_string(customCanvasHeightMillimeters) + " MM";
        case CanvasPreset::None:
            return "--";
    }
    return "--";
}

std::string ProteusApplication::createTextureKey(const std::string& text,
                                                 const SDL_Color& color) const
{
    return text + "#" + std::to_string(color.r) + ":" +
           std::to_string(color.g) + ":" + std::to_string(color.b) + ":" +
           std::to_string(color.a);
}

SDL_Texture* ProteusApplication::textTexture(const std::string& text,
                                             const SDL_Color& color)
{
    if (text.empty())
    {
        return nullptr;
    }

    const std::string key = createTextureKey(text, color);
    const auto found = textTextureCache.find(key);
    if (found != textTextureCache.end())
    {
        return found->second;
    }

    if (textTextureCache.size() >= 512)
    {
        clearTextCache();
    }

    const int rawWidth = std::max(1, static_cast<int>(text.size()) * 8);
    constexpr int rawHeight = 8;
    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_TARGET,
                                             rawWidth,
                                             rawHeight);
    if (texture == nullptr)
    {
        return nullptr;
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_Texture* previousTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, texture);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    stringRGBA(renderer, 0, 0, text.c_str(),
               color.r, color.g, color.b, color.a);
    SDL_SetRenderTarget(renderer, previousTarget);
    textTextureCache[key] = texture;
    return texture;
}

void ProteusApplication::clearTextCache()
{
    for (auto& item : textTextureCache)
    {
        SDL_DestroyTexture(item.second);
    }
    textTextureCache.clear();
}

void ProteusApplication::drawPixelTextAt(const std::string& text,
                                         int x,
                                         int y,
                                         int scale,
                                         const SDL_Color& color)
{
    SDL_Texture* texture = textTexture(text, color);
    if (texture == nullptr)
    {
        return;
    }

    const SDL_Rect destination{x,
                               y,
                               static_cast<int>(text.size()) * 8 * scale,
                               8 * scale};
    SDL_RenderCopy(renderer, texture, nullptr, &destination);
}

void ProteusApplication::drawPixelTextCentered(const std::string& text,
                                               const SDL_Rect& area,
                                               int preferredScale,
                                               const SDL_Color& color)
{
    if (text.empty())
    {
        return;
    }

    int scale = std::max(1, preferredScale);
    while (scale > 1 && static_cast<int>(text.size()) * 8 * scale > area.w - 12)
    {
        --scale;
    }

    const int width = static_cast<int>(text.size()) * 8 * scale;
    const int height = 8 * scale;
    drawPixelTextAt(text,
                    area.x + (area.w - width) / 2,
                    area.y + (area.h - height) / 2,
                    scale,
                    color);
}

void ProteusApplication::drawFilledRectangle(const SDL_Rect& rectangle,
                                             const SDL_Color& fill,
                                             const SDL_Color& border,
                                             int borderThickness)
{
    boxRGBA(renderer,
            static_cast<Sint16>(rectangle.x),
            static_cast<Sint16>(rectangle.y),
            static_cast<Sint16>(rectangle.x + rectangle.w - 1),
            static_cast<Sint16>(rectangle.y + rectangle.h - 1),
            fill.r, fill.g, fill.b, fill.a);
    drawOutlineRectangle(rectangle, border, borderThickness);
}

void ProteusApplication::drawOutlineRectangle(const SDL_Rect& rectangle,
                                              const SDL_Color& color,
                                              int borderThickness)
{
    for (int offset = 0; offset < borderThickness; ++offset)
    {
        rectangleRGBA(renderer,
                      static_cast<Sint16>(rectangle.x + offset),
                      static_cast<Sint16>(rectangle.y + offset),
                      static_cast<Sint16>(rectangle.x + rectangle.w - 1 - offset),
                      static_cast<Sint16>(rectangle.y + rectangle.h - 1 - offset),
                      color.r, color.g, color.b, color.a);
    }
}

void ProteusApplication::render()
{
    SDL_SetRenderTarget(renderer, nullptr);

    if (currentPage == Page::DesignWorkspace)
    {
        renderDesignWorkspace();
    }
    else
    {
        renderMenuPage();
    }

    if (customSizeDialog.open)
    {
        renderCustomSizeDialog();
    }
    if (propertiesDialog.open)
    {
        renderPropertiesDialog();
    }

    SDL_RenderPresent(renderer);
}

void ProteusApplication::renderMenuPage()
{
    SDL_SetRenderDrawColor(renderer,
                           backgroundColor.r,
                           backgroundColor.g,
                           backgroundColor.b,
                           backgroundColor.a);
    SDL_RenderClear(renderer);

    for (const Button& button : menuButtons)
    {
        if (button.type() == ButtonType::PageTitle)
        {
            drawFilledRectangle(button.bounds(), titleColor, borderColor, 3);
            drawPixelTextCentered(button.text(), button.bounds(), 4, whiteText);
        }
        else
        {
            const SDL_Color& fill = button.isHovered() ? hoverColor : buttonColor;
            drawFilledRectangle(button.bounds(), fill, borderColor, 2);
            drawPixelTextCentered(button.text(), button.bounds(), 3, whiteText);
        }
    }

    if (currentPage == Page::NewProjectMenu)
    {
        const SDL_Rect hint{385, 600, 590, 42};
        drawPixelTextCentered("ESC: BACK TO MAIN MENU", hint, 2, titleColor);
    }
}

void ProteusApplication::renderDesignWorkspace()
{
    SDL_SetRenderDrawColor(renderer,
                           workspaceBackground.r,
                           workspaceBackground.g,
                           workspaceBackground.b,
                           workspaceBackground.a);
    SDL_RenderClear(renderer);

    const SDL_Rect canvas = canvasArea();
    drawFilledRectangle(canvas, canvasColor, borderColor, 2);
    renderGrid(canvas);
    renderCanvas(canvas);
    renderTopToolbar(toolbarArea());
    renderLeftComponentPanel(leftPanelArea());
    renderRightPropertiesPanel(rightPanelArea());
    renderStatusBar(statusBarArea());

    if (contextMenuOpen)
    {
        renderContextMenu();
    }
}

void ProteusApplication::rebuildToolbarHitAreas()
{
    toolbarButtons.clear();
    constexpr int y = 10;
    constexpr int height = 38;
    int x = 14;

    const std::pair<const char*, ToolbarAction> definitions[] = {
        {"SELECT", ToolbarAction::Select},
        {"ROTATE", ToolbarAction::Rotate},
        {"MIRROR H", ToolbarAction::MirrorHorizontal},
        {"MIRROR V", ToolbarAction::MirrorVertical},
        {"DELETE", ToolbarAction::Delete},
        {"SAVE", ToolbarAction::Save},
        {"UNDO", ToolbarAction::Undo},
        {"REDO", ToolbarAction::Redo},
        {"DRC", ToolbarAction::Drc},
        {"RUN", ToolbarAction::RunPause},
        {"STOP", ToolbarAction::Stop},
        {"RESET VIEW", ToolbarAction::ResetView}
    };

    for (const auto& definition : definitions)
    {
        const int width = std::max(62,
                                   static_cast<int>(std::string(definition.first).size()) * 8 + 16);
        toolbarButtons.push_back({{x, y, width, height}, definition.second});
        x += width + 5;
    }
}

void ProteusApplication::renderTopToolbar(const SDL_Rect& toolbar)
{
    drawFilledRectangle(toolbar, titleColor, borderColor, 2);
    rebuildToolbarHitAreas();

    const char* labels[] = {"SELECT", "ROTATE", "MIRROR H", "MIRROR V",
                            "DELETE", "SAVE", "UNDO", "REDO", "DRC",
                            "RUN", "STOP", "RESET VIEW"};

    for (std::size_t index = 0; index < toolbarButtons.size(); ++index)
    {
        const bool isSelectActive = toolbarButtons[index].second == ToolbarAction::Select &&
                                    placementDefinitionId < 0;
        const SDL_Color fill = isSelectActive ? selectedItemColor : buttonColor;
        drawFilledRectangle(toolbarButtons[index].first, fill, whiteText, 1);
        const std::string label = toolbarButtons[index].second == ToolbarAction::RunPause &&
                                  simulationRunning
                                  ? "PAUSE"
                                  : labels[index];
        drawPixelTextCentered(label, toolbarButtons[index].first, 1, whiteText);
    }

    const SDL_Rect titleArea{LOGICAL_WIDTH - 188, 0, 176, TOOLBAR_HEIGHT};
    drawPixelTextCentered("PROTEUS", titleArea, 2, whiteText);
}

void ProteusApplication::rebuildLeftPanelHitAreas()
{
    categoryRows.clear();
    libraryRows.clear();
    activeRows.clear();

    const SDL_Rect panel = leftPanelArea();
    searchBox = {panel.x + 12, panel.y + 50, panel.w - 24, 32};

    const int categoryWidth = (panel.w - 34) / 2;
    int categoryIndex = 0;
    for (const std::string& category : componentLibrary.categories())
    {
        const int column = categoryIndex % 2;
        const int row = categoryIndex / 2;
        categoryRows.push_back({{panel.x + 14 + column * (categoryWidth + 6),
                                 panel.y + 106 + row * 24,
                                 categoryWidth,
                                 22},
                                category});
        ++categoryIndex;
    }

    const std::vector<int> filtered = componentLibrary.filteredIds(selectedCategory,
                                                                    searchText);
    const int maximumLibraryOffset = std::max(
            0, static_cast<int>(filtered.size()) - MAX_LIBRARY_ROWS);
    libraryScrollOffset = std::clamp(libraryScrollOffset,
                                     0,
                                     maximumLibraryOffset);

    int libraryY = panel.y + 222;
    for (int index = libraryScrollOffset;
         index < static_cast<int>(filtered.size()) &&
         index < libraryScrollOffset + MAX_LIBRARY_ROWS;
         ++index)
    {
        libraryRows.push_back({{panel.x + 14, libraryY, panel.w - 28, 27},
                               {},
                               filtered[index]});
        libraryY += 29;
    }

    addToActiveButton = {panel.x + 139, panel.y + 474, panel.w - 153, 27};

    const int maximumActiveOffset = std::max(
            0, static_cast<int>(activeDefinitionIds.size()) - MAX_ACTIVE_ROWS);
    activeScrollOffset = std::clamp(activeScrollOffset, 0, maximumActiveOffset);

    int activeY = panel.y + 542;
    for (int index = activeScrollOffset;
         index < static_cast<int>(activeDefinitionIds.size()) &&
         index < activeScrollOffset + MAX_ACTIVE_ROWS;
         ++index)
    {
        const SDL_Rect row{panel.x + 14, activeY, panel.w - 28, 31};
        activeRows.push_back({row,
                              {row.x + row.w - 32, row.y + 3, 27, row.h - 6},
                              activeDefinitionIds[index]});
        activeY += 34;
    }
}

void ProteusApplication::renderLeftComponentPanel(const SDL_Rect& leftPanel)
{
    drawFilledRectangle(leftPanel, panelColor, borderColor, 2);
    rebuildLeftPanelHitAreas();

    drawPixelTextCentered("COMPONENT LIBRARY",
                          {leftPanel.x + 8, leftPanel.y + 10, leftPanel.w - 16, 30},
                          2,
                          whiteText);

    drawFilledRectangle(searchBox,
                        searchFocused ? SDL_Color{255, 255, 255, 255}
                                      : SDL_Color{221, 237, 244, 255},
                        searchFocused ? selectionColor : borderColor,
                        searchFocused ? 2 : 1);

    const std::string searchDisplay = searchText.empty()
                                      ? "SEARCH NAME/CATEGORY"
                                      : searchText + (searchFocused ? "_" : "");
    drawPixelTextAt(searchDisplay,
                    searchBox.x + 8,
                    searchBox.y + 10,
                    1,
                    darkText);

    drawPixelTextAt("CATEGORIES", leftPanel.x + 15, leftPanel.y + 90, 1, whiteText);
    for (const auto& categoryRow : categoryRows)
    {
        const bool selected = categoryRow.second == selectedCategory;
        drawFilledRectangle(categoryRow.first,
                            selected ? selectedItemColor : panelItemColor,
                            selected ? whiteText : borderColor,
                            1);
        drawPixelTextAt((selected ? "- " : "+ ") + categoryRow.second,
                        categoryRow.first.x + 7,
                        categoryRow.first.y + 7,
                        1,
                        whiteText);
    }

    drawPixelTextAt("RESULTS", leftPanel.x + 15, leftPanel.y + 206, 1, whiteText);
    if (libraryRows.empty())
    {
        drawPixelTextCentered("NO COMPONENT FOUND",
                              {leftPanel.x + 14, leftPanel.y + 240,
                               leftPanel.w - 28, 52},
                              1,
                              whiteText);
    }

    for (const ComponentRow& row : libraryRows)
    {
        const ComponentDefinition* definition = componentLibrary.findById(
                row.definitionId);
        if (definition == nullptr)
        {
            continue;
        }

        const bool selected = previewDefinitionId == row.definitionId;
        drawFilledRectangle(row.bounds,
                            selected ? selectedItemColor : panelItemColor,
                            selected ? whiteText : borderColor,
                            1);
        drawPixelTextAt(definition->name,
                        row.bounds.x + 8,
                        row.bounds.y + 9,
                        1,
                        whiteText);
        if (isDefinitionActive(row.definitionId))
        {
            drawPixelTextAt("ACTIVE",
                            row.bounds.x + row.bounds.w - 55,
                            row.bounds.y + 9,
                            1,
                            SDL_Color{194, 255, 188, 255});
        }
    }

    const SDL_Rect previewArea{leftPanel.x + 14,
                               leftPanel.y + 350,
                               leftPanel.w - 28,
                               116};
    drawFilledRectangle(previewArea,
                        SDL_Color{229, 240, 245, 255},
                        borderColor,
                        1);
    const ComponentDefinition* preview = componentLibrary.findById(
            previewDefinitionId);
    if (preview != nullptr)
    {
        renderDefinitionPreview(*preview,
                                {previewArea.x + 4,
                                 previewArea.y + 4,
                                 previewArea.w - 8,
                                 68});
        drawPixelTextAt(preview->category,
                        previewArea.x + 8,
                        previewArea.y + 84,
                        1,
                        darkText);
        drawFilledRectangle(addToActiveButton,
                            isDefinitionActive(previewDefinitionId)
                            ? SDL_Color{100, 130, 140, 255}
                            : buttonColor,
                            borderColor,
                            1);
        drawPixelTextCentered(isDefinitionActive(previewDefinitionId)
                              ? "ADDED"
                              : "ADD",
                              addToActiveButton,
                              1,
                              whiteText);
    }

    drawPixelTextAt("ACTIVE COMPONENTS",
                    leftPanel.x + 15,
                    leftPanel.y + 521,
                    1,
                    whiteText);

    if (activeRows.empty())
    {
        drawPixelTextCentered("LIST IS EMPTY",
                              {leftPanel.x + 14, leftPanel.y + 552,
                               leftPanel.w - 28, 44},
                              1,
                              whiteText);
    }

    for (const ComponentRow& row : activeRows)
    {
        const ComponentDefinition* definition = componentLibrary.findById(
                row.definitionId);
        if (definition == nullptr)
        {
            continue;
        }

        const bool armed = placementDefinitionId == row.definitionId;
        drawFilledRectangle(row.bounds,
                            armed ? selectionColor : panelItemColor,
                            borderColor,
                            1);
        drawPixelTextAt(definition->name,
                        row.bounds.x + 8,
                        row.bounds.y + 11,
                        1,
                        whiteText);
        drawFilledRectangle(row.removeBounds,
                            SDL_Color{150, 48, 48, 255},
                            borderColor,
                            1);
        drawPixelTextCentered("X", row.removeBounds, 1, whiteText);
    }
}

void ProteusApplication::renderRightPropertiesPanel(const SDL_Rect& rightPanel)
{
    drawFilledRectangle(rightPanel, panelColor, borderColor, 2);
    drawPixelTextCentered("PROPERTIES",
                          {rightPanel.x + 10, rightPanel.y + 12,
                           rightPanel.w - 20, 34},
                          2,
                          whiteText);

    const SDL_Rect selectionBox{rightPanel.x + 14,
                                rightPanel.y + 58,
                                rightPanel.w - 28,
                                48};
    drawFilledRectangle(selectionBox, panelItemColor, borderColor, 1);

    if (selectedComponentIds.empty())
    {
        drawPixelTextCentered("NO COMPONENT SELECTED",
                              selectionBox,
                              1,
                              whiteText);
        drawPixelTextCentered("DOUBLE CLICK A COMPONENT",
                              {rightPanel.x + 14, rightPanel.y + 135,
                               rightPanel.w - 28, 40},
                              1,
                              whiteText);
        editPropertiesButton = {};
        return;
    }

    if (selectedComponentIds.size() > 1)
    {
        drawPixelTextCentered(std::to_string(selectedComponentIds.size()) +
                              " COMPONENTS SELECTED",
                              selectionBox,
                              1,
                              whiteText);
        drawPixelTextAt("R: ROTATE ALL",
                        rightPanel.x + 20,
                        rightPanel.y + 145,
                        1,
                        whiteText);
        drawPixelTextAt("H/V: MIRROR ALL",
                        rightPanel.x + 20,
                        rightPanel.y + 177,
                        1,
                        whiteText);
        drawPixelTextAt("DELETE: REMOVE ALL",
                        rightPanel.x + 20,
                        rightPanel.y + 209,
                        1,
                        whiteText);
        editPropertiesButton = {};
        return;
    }

    const ComponentInstance* component = componentById(*selectedComponentIds.begin());
    if (component == nullptr)
    {
        return;
    }

    const ComponentDefinition* definition = componentLibrary.findById(
            component->definitionId());
    if (definition == nullptr)
    {
        return;
    }

    drawPixelTextCentered(definition->name, selectionBox, 2, whiteText);

    int y = rightPanel.y + 135;
    const int x = rightPanel.x + 20;
    drawPixelTextAt("LABEL: " + component->label(), x, y, 1, whiteText);
    y += 34;
    drawPixelTextAt(definition->valueCaption + ":", x, y, 1, whiteText);
    y += 22;
    drawPixelTextAt(component->value(), x + 14, y, 1, whiteText);
    y += 38;
    drawPixelTextAt("ROTATION: " + std::to_string(component->rotationDegrees()),
                    x, y, 1, whiteText);
    y += 34;
    drawPixelTextAt("MIRROR H: " +
                    std::string(component->isMirroredHorizontally() ? "YES" : "NO"),
                    x, y, 1, whiteText);
    y += 34;
    drawPixelTextAt("MIRROR V: " +
                    std::string(component->isMirroredVertically() ? "YES" : "NO"),
                    x, y, 1, whiteText);
    y += 34;

    const WorldPoint position = component->position();
    drawPixelTextAt("POSITION: " + std::to_string(static_cast<int>(position.x)) +
                    ", " + std::to_string(static_cast<int>(position.y)),
                    x, y, 1, whiteText);
    y += 36;
    drawPixelTextAt("PINS:", x, y, 1, whiteText);
    y += 24;

    for (std::size_t pinIndex = 0;
         pinIndex < definition->pins.size() && pinIndex < 4;
         ++pinIndex)
    {
        const WorldPoint pin = component->pinPosition(*definition, pinIndex);
        drawPixelTextAt(definition->pins[pinIndex].name + " = " +
                        std::to_string(static_cast<int>(std::lround(pin.x))) +
                        ", " +
                        std::to_string(static_cast<int>(std::lround(pin.y))),
                        x + 12,
                        y,
                        1,
                        whiteText);
        y += 24;
    }

    editPropertiesButton = {rightPanel.x + 28,
                            rightPanel.y + rightPanel.h - 70,
                            rightPanel.w - 56,
                            40};
    drawFilledRectangle(editPropertiesButton, buttonColor, whiteText, 1);
    drawPixelTextCentered("EDIT PROPERTIES", editPropertiesButton, 1, whiteText);
}

void ProteusApplication::renderCanvas(const SDL_Rect& canvas)
{
    SDL_RenderSetClipRect(renderer, &canvas);

    renderWires();

    for (const ComponentInstance& component : components)
    {
        renderComponent(component);
    }

    renderPlacementPreview();
    renderPendingWire();
    renderSelectionRectangle();

    const SDL_Rect tag{canvas.x + 12, canvas.y + 12, 210, 30};
    drawFilledRectangle(tag, titleColor, borderColor, 1);
    drawPixelTextCentered(presetName() + " DESIGN CANVAS", tag, 1, whiteText);

    SDL_RenderSetClipRect(renderer, nullptr);
}

void ProteusApplication::renderWires()
{
    for (const WireView& wire : backend.wireViews())
    {
        SDL_Color color{72, 91, 102, 255};
        if (wire.id == selectedWireId)
        {
            color = selectionColor;
        }
        else
        {
            switch (wire.state)
            {
                case WireLogicState::Low:
                    color = SDL_Color{35, 92, 168, 255};
                    break;
                case WireLogicState::High:
                    color = SDL_Color{25, 150, 82, 255};
                    break;
                case WireLogicState::Undefined:
                    color = SDL_Color{190, 55, 55, 255};
                    break;
                case WireLogicState::Floating:
                    break;
            }
        }

        for (std::size_t index = 1; index < wire.points.size(); ++index)
        {
            const SDL_Point first = worldToScreen(wire.points[index - 1]);
            const SDL_Point second = worldToScreen(wire.points[index]);
            thickLineRGBA(renderer,
                          static_cast<Sint16>(first.x),
                          static_cast<Sint16>(first.y),
                          static_cast<Sint16>(second.x),
                          static_cast<Sint16>(second.y),
                          wire.id == selectedWireId ? 4 : 3,
                          color.r, color.g, color.b, color.a);
        }
    }

    for (const WorldPoint& junction : backend.junctionPositions())
    {
        const SDL_Point point = worldToScreen(junction);
        filledCircleRGBA(renderer,
                         static_cast<Sint16>(point.x),
                         static_cast<Sint16>(point.y),
                         5,
                         darkText.r, darkText.g, darkText.b, 255);
    }
}

void ProteusApplication::renderPendingWire()
{
    if (!pendingWireStart.has_value() || !mouseInsideCanvas)
    {
        return;
    }
    const std::optional<WorldPoint> start = backend.pinPosition(*pendingWireStart);
    if (!start.has_value())
    {
        return;
    }

    const SDL_Point first = worldToScreen(*start);
    const SDL_Point corner = worldToScreen({canvasMouseWorld.x, start->y});
    const SDL_Point last = worldToScreen(canvasMouseWorld);
    thickLineRGBA(renderer, first.x, first.y, corner.x, corner.y, 2,
                  selectionColor.r, selectionColor.g, selectionColor.b, 180);
    thickLineRGBA(renderer, corner.x, corner.y, last.x, last.y, 2,
                  selectionColor.r, selectionColor.g, selectionColor.b, 180);
}

void ProteusApplication::renderGrid(const SDL_Rect& canvas)
{
    SDL_RenderSetClipRect(renderer, &canvas);
    const WorldPoint topLeft = screenToWorld(canvas.x, canvas.y);
    const WorldPoint bottomRight = screenToWorld(canvas.x + canvas.w,
                                                 canvas.y + canvas.h);

    const int firstVertical = static_cast<int>(
            std::floor(std::min(topLeft.x, bottomRight.x) / GRID_SPACING));
    const int lastVertical = static_cast<int>(
            std::ceil(std::max(topLeft.x, bottomRight.x) / GRID_SPACING));

    for (int index = firstVertical; index <= lastVertical; ++index)
    {
        const SDL_Point screen = worldToScreen(
                {static_cast<double>(index * GRID_SPACING), 0.0});
        const SDL_Color& color = index % 5 == 0 ? majorGridColor : gridColor;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer,
                           screen.x,
                           canvas.y,
                           screen.x,
                           canvas.y + canvas.h - 1);
    }

    const int firstHorizontal = static_cast<int>(
            std::floor(std::min(topLeft.y, bottomRight.y) / GRID_SPACING));
    const int lastHorizontal = static_cast<int>(
            std::ceil(std::max(topLeft.y, bottomRight.y) / GRID_SPACING));

    for (int index = firstHorizontal; index <= lastHorizontal; ++index)
    {
        const SDL_Point screen = worldToScreen(
                {0.0, static_cast<double>(index * GRID_SPACING)});
        const SDL_Color& color = index % 5 == 0 ? majorGridColor : gridColor;
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer,
                           canvas.x,
                           screen.y,
                           canvas.x + canvas.w - 1,
                           screen.y);
    }

    const SDL_Point origin = worldToScreen({0.0, 0.0});
    lineRGBA(renderer,
             static_cast<Sint16>(origin.x - 7),
             static_cast<Sint16>(origin.y),
             static_cast<Sint16>(origin.x + 7),
             static_cast<Sint16>(origin.y),
             titleColor.r, titleColor.g, titleColor.b, 255);
    lineRGBA(renderer,
             static_cast<Sint16>(origin.x),
             static_cast<Sint16>(origin.y - 7),
             static_cast<Sint16>(origin.x),
             static_cast<Sint16>(origin.y + 7),
             titleColor.r, titleColor.g, titleColor.b, 255);
    SDL_RenderSetClipRect(renderer, nullptr);
}

void ProteusApplication::renderStatusBar(const SDL_Rect& statusBar)
{
    drawFilledRectangle(statusBar, titleColor, borderColor, 2);

    std::string mouseText = "X: --   Y: --";
    if (mouseInsideCanvas)
    {
        mouseText = "X: " +
                    std::to_string(static_cast<int>(std::lround(canvasMouseWorld.x))) +
                    "   Y: " +
                    std::to_string(static_cast<int>(std::lround(canvasMouseWorld.y)));
    }

    drawPixelTextAt(mouseText, 16, statusBar.y + 14, 1, whiteText);
    drawPixelTextAt("ZOOM: " +
                    std::to_string(static_cast<int>(std::lround(zoom * 100.0))) + "%",
                    250,
                    statusBar.y + 14,
                    1,
                    whiteText);
    drawPixelTextAt("GRID: 20 PX", 390, statusBar.y + 14, 1, whiteText);

    std::string mode;
    if (pendingWireStart.has_value())
    {
        mode = "MODE: WIRE - SELECT SECOND PIN";
    }
    else if (placementDefinitionId >= 0)
    {
        mode = "MODE: PLACE - ESC TO CANCEL";
    }
    else
    {
        mode = simulationRunning ? "MODE: SIMULATION RUNNING" : "MODE: SELECT / WIRE";
    }
    drawPixelTextAt(mode, 520, statusBar.y + 14, 1, whiteText);

    drawPixelTextAt(statusMessage.substr(0, 36),
                    860,
                    statusBar.y + 14,
                    1,
                    whiteText);
}

void ProteusApplication::drawSymbol(
        const ComponentDefinition& definition,
        const std::function<SDL_Point(const WorldPoint&)>& mapper,
        const SDL_Color& symbolColor,
        bool drawPins)
{
    const auto drawLine = [this, &mapper, &symbolColor](const WorldPoint& first,
                                                        const WorldPoint& second,
                                                        int thickness = 2)
    {
        const SDL_Point a = mapper(first);
        const SDL_Point b = mapper(second);
        thickLineRGBA(renderer,
                      static_cast<Sint16>(a.x),
                      static_cast<Sint16>(a.y),
                      static_cast<Sint16>(b.x),
                      static_cast<Sint16>(b.y),
                      static_cast<Uint8>(thickness),
                      symbolColor.r,
                      symbolColor.g,
                      symbolColor.b,
                      symbolColor.a);
    };

    const auto drawPolyline = [&drawLine](const std::vector<WorldPoint>& points)
    {
        for (std::size_t index = 1; index < points.size(); ++index)
        {
            drawLine(points[index - 1], points[index]);
        }
    };

    switch (definition.symbol)
    {
        case SymbolKind::Resistor:
            drawLine({-45.0, 0.0}, {-32.0, 0.0});
            drawPolyline({{-32.0, 0.0}, {-25.0, -12.0}, {-15.0, 12.0},
                          {-5.0, -12.0}, {5.0, 12.0}, {15.0, -12.0},
                          {25.0, 12.0}, {32.0, 0.0}});
            drawLine({32.0, 0.0}, {45.0, 0.0});
            break;

        case SymbolKind::Capacitor:
            drawLine({-40.0, 0.0}, {-9.0, 0.0});
            drawLine({-9.0, -22.0}, {-9.0, 22.0}, 3);
            drawLine({9.0, -22.0}, {9.0, 22.0}, 3);
            drawLine({9.0, 0.0}, {40.0, 0.0});
            break;

        case SymbolKind::Inductor:
            drawLine({-45.0, 0.0}, {-32.0, 0.0});
            drawPolyline({{-32.0, 0.0}, {-25.0, -13.0}, {-15.0, 13.0},
                          {-5.0, -13.0}, {5.0, 13.0}, {15.0, -13.0},
                          {25.0, 13.0}, {32.0, 0.0}});
            drawLine({32.0, 0.0}, {45.0, 0.0});
            break;

        case SymbolKind::Battery:
            drawLine({-41.0, 0.0}, {-10.0, 0.0});
            drawLine({-10.0, -16.0}, {-10.0, 16.0}, 3);
            drawLine({10.0, -26.0}, {10.0, 26.0}, 3);
            drawLine({10.0, 0.0}, {41.0, 0.0});
            break;

        case SymbolKind::Led:
        {
            drawLine({-43.0, 0.0}, {-21.0, 0.0});
            const SDL_Point a = mapper({-21.0, -19.0});
            const SDL_Point b = mapper({-21.0, 19.0});
            const SDL_Point c = mapper({16.0, 0.0});
            trigonRGBA(renderer,
                       static_cast<Sint16>(a.x), static_cast<Sint16>(a.y),
                       static_cast<Sint16>(b.x), static_cast<Sint16>(b.y),
                       static_cast<Sint16>(c.x), static_cast<Sint16>(c.y),
                       symbolColor.r, symbolColor.g, symbolColor.b, symbolColor.a);
            drawLine({16.0, -21.0}, {16.0, 21.0}, 3);
            drawLine({16.0, 0.0}, {43.0, 0.0});
            drawLine({1.0, -22.0}, {13.0, -34.0});
            drawLine({12.0, -24.0}, {24.0, -36.0});
            break;
        }

        case SymbolKind::Ground:
            drawLine({0.0, -31.0}, {0.0, 6.0});
            drawLine({-25.0, 6.0}, {25.0, 6.0}, 3);
            drawLine({-17.0, 15.0}, {17.0, 15.0}, 3);
            drawLine({-9.0, 24.0}, {9.0, 24.0}, 3);
            break;

        case SymbolKind::Switch:
            drawLine({-45.0, 0.0}, {-26.0, 0.0});
            drawLine({-22.0, -2.0}, {22.0, -17.0}, 3);
            drawLine({26.0, 0.0}, {45.0, 0.0});
            {
                const SDL_Point firstContact = mapper({-24.0, 0.0});
                const SDL_Point secondContact = mapper({24.0, 0.0});
                filledCircleRGBA(renderer,
                                 static_cast<Sint16>(firstContact.x),
                                 static_cast<Sint16>(firstContact.y),
                                 4,
                                 symbolColor.r, symbolColor.g,
                                 symbolColor.b, symbolColor.a);
                filledCircleRGBA(renderer,
                                 static_cast<Sint16>(secondContact.x),
                                 static_cast<Sint16>(secondContact.y),
                                 4,
                                 symbolColor.r, symbolColor.g,
                                 symbolColor.b, symbolColor.a);
            }
            break;

        case SymbolKind::AndGate:
        {
            drawLine({-46.0, -18.0}, {-32.0, -18.0});
            drawLine({-46.0, 18.0}, {-32.0, 18.0});
            drawLine({32.0, 0.0}, {46.0, 0.0});
            drawPolyline({{-32.0, -30.0}, {-5.0, -30.0},
                          {8.0, -27.0}, {20.0, -19.0},
                          {29.0, -8.0}, {32.0, 0.0},
                          {29.0, 8.0}, {20.0, 19.0},
                          {8.0, 27.0}, {-5.0, 30.0},
                          {-32.0, 30.0}, {-32.0, -30.0}});
            const SDL_Point center = mapper({0.0, 0.0});
            drawPixelTextAt("AND", center.x - 12, center.y - 4, 1, symbolColor);
            break;
        }

        case SymbolKind::Clock:
        {
            drawLine({-41.0, 0.0}, {-31.0, 0.0});
            drawLine({31.0, 0.0}, {41.0, 0.0});
            drawPolyline({{-31.0, -25.0}, {31.0, -25.0},
                          {31.0, 25.0}, {-31.0, 25.0},
                          {-31.0, -25.0}});
            drawPolyline({{-23.0, 9.0}, {-13.0, 9.0}, {-13.0, -9.0},
                          {1.0, -9.0}, {1.0, 9.0}, {14.0, 9.0},
                          {14.0, -9.0}, {23.0, -9.0}});
            break;
        }

        case SymbolKind::GenericBlock:
        {
            const double halfWidth = definition.width / 2.0;
            const double halfHeight = definition.height / 2.0;
            drawPolyline({{-halfWidth, -halfHeight}, {halfWidth, -halfHeight},
                          {halfWidth, halfHeight}, {-halfWidth, halfHeight},
                          {-halfWidth, -halfHeight}});
            const SDL_Point center = mapper({0.0, 0.0});
            drawPixelTextCentered(definition.prefix,
                                  {center.x - 42, center.y - 11, 84, 22},
                                  1,
                                  symbolColor);
            break;
        }
    }

    if (!drawPins)
    {
        return;
    }

    for (const PinDefinition& pin : definition.pins)
    {
        const SDL_Point position = mapper(pin.localPosition);
        filledCircleRGBA(renderer,
                         static_cast<Sint16>(position.x),
                         static_cast<Sint16>(position.y),
                         4,
                         pinColor.r, pinColor.g, pinColor.b, symbolColor.a);
        circleRGBA(renderer,
                   static_cast<Sint16>(position.x),
                   static_cast<Sint16>(position.y),
                   5,
                   255, 255, 255, symbolColor.a);
    }
}

void ProteusApplication::renderComponent(const ComponentInstance& component)
{
    const ComponentDefinition* definition = componentLibrary.findById(
            component.definitionId());
    if (definition == nullptr)
    {
        return;
    }

    const bool selected = selectedComponentIds.count(component.id()) != 0;
    const SDL_Color color = selected ? selectionColor : darkText;
    const auto mapper = [this, &component](const WorldPoint& localPoint)
    {
        return worldToScreen(component.transformLocalPoint(localPoint));
    };

    drawSymbol(*definition, mapper, color, true);
    const SDL_Rect bounds = componentScreenBounds(component, *definition);
    drawPixelTextCentered(component.label() + "  " + component.value(),
                          {bounds.x - 25, bounds.y - 23, bounds.w + 50, 18},
                          1,
                          color);

    if (selected)
    {
        const SDL_Rect highlight{bounds.x - 6,
                                 bounds.y - 6,
                                 bounds.w + 12,
                                 bounds.h + 12};
        drawOutlineRectangle(highlight, selectionColor, 2);
    }
}

void ProteusApplication::renderPlacementPreview()
{
    if (placementDefinitionId < 0 || !mouseInsideCanvas)
    {
        return;
    }

    const ComponentDefinition* definition = componentLibrary.findById(
            placementDefinitionId);
    if (definition == nullptr)
    {
        return;
    }

    const ComponentInstance preview(-1,
                                    placementDefinitionId,
                                    "",
                                    "",
                                    snapPoint(canvasMouseWorld));
    const auto mapper = [this, &preview](const WorldPoint& localPoint)
    {
        return worldToScreen(preview.transformLocalPoint(localPoint));
    };

    drawSymbol(*definition,
               mapper,
               SDL_Color{selectionColor.r, selectionColor.g,
                         selectionColor.b, 145},
               true);
    const SDL_Rect bounds = componentScreenBounds(preview, *definition);
    drawOutlineRectangle({bounds.x - 5, bounds.y - 5,
                          bounds.w + 10, bounds.h + 10},
                         SDL_Color{selectionColor.r, selectionColor.g,
                                   selectionColor.b, 150},
                         1);
}

void ProteusApplication::renderDefinitionPreview(
        const ComponentDefinition& definition,
        const SDL_Rect& previewArea)
{
    const double scaleX = static_cast<double>(previewArea.w - 30) /
                          std::max(1.0, definition.width);
    const double scaleY = static_cast<double>(previewArea.h - 16) /
                          std::max(1.0, definition.height);
    const double previewScale = std::min({1.05, scaleX, scaleY});
    const auto mapper = [previewArea, previewScale](const WorldPoint& localPoint)
    {
        return SDL_Point{
            previewArea.x + previewArea.w / 2 +
            static_cast<int>(std::lround(localPoint.x * previewScale)),
            previewArea.y + previewArea.h / 2 +
            static_cast<int>(std::lround(localPoint.y * previewScale))
        };
    };

    drawSymbol(definition, mapper, darkText, true);
    drawPixelTextCentered(definition.name,
                          {previewArea.x,
                           previewArea.y + previewArea.h - 15,
                           previewArea.w,
                           16},
                          1,
                          darkText);
}

void ProteusApplication::renderSelectionRectangle()
{
    if (mouseOperation != MouseOperation::BoxSelect)
    {
        return;
    }

    const SDL_Rect rectangle = normalizedRectangle(selectionStartScreen,
                                                   selectionCurrentScreen);
    boxRGBA(renderer,
            static_cast<Sint16>(rectangle.x),
            static_cast<Sint16>(rectangle.y),
            static_cast<Sint16>(rectangle.x + rectangle.w),
            static_cast<Sint16>(rectangle.y + rectangle.h),
            selectionColor.r, selectionColor.g, selectionColor.b, 45);
    drawOutlineRectangle(rectangle, selectionColor, 1);
}

void ProteusApplication::renderContextMenu()
{
    constexpr int itemHeight = 30;
    const SDL_Rect menu{contextMenuPosition.x,
                        contextMenuPosition.y,
                        174,
                        itemHeight * 5};
    drawFilledRectangle(menu,
                        SDL_Color{245, 249, 251, 255},
                        borderColor,
                        2);

    const char* items[] = {"ROTATE 90", "MIRROR H", "MIRROR V",
                           "PROPERTIES", "DELETE"};
    for (int index = 0; index < 5; ++index)
    {
        const SDL_Rect item{menu.x + 3,
                            menu.y + index * itemHeight + 2,
                            menu.w - 6,
                            itemHeight - 3};
        if (pointInsideRectangle(lastMouseX, lastMouseY, item))
        {
            boxRGBA(renderer,
                    static_cast<Sint16>(item.x),
                    static_cast<Sint16>(item.y),
                    static_cast<Sint16>(item.x + item.w),
                    static_cast<Sint16>(item.y + item.h),
                    205, 226, 235, 255);
        }
        drawPixelTextAt(items[index], item.x + 8, item.y + 9, 1, darkText);
    }
}

void ProteusApplication::renderPropertiesDialog()
{
    boxRGBA(renderer, 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT, 0, 20, 30, 150);
    const SDL_Rect dialog{420, 205, 520, 390};
    drawFilledRectangle(dialog,
                        SDL_Color{235, 244, 248, 255},
                        borderColor,
                        3);

    const ComponentInstance* component = componentById(propertiesDialog.componentId);
    if (component == nullptr)
    {
        closePropertiesDialog(false);
        return;
    }

    const ComponentDefinition* definition = componentLibrary.findById(
            component->definitionId());
    if (definition == nullptr)
    {
        closePropertiesDialog(false);
        return;
    }

    const SDL_Rect heading{dialog.x, dialog.y, dialog.w, 60};
    drawFilledRectangle(heading, titleColor, borderColor, 2);
    drawPixelTextCentered(definition->name + " PROPERTIES",
                          heading,
                          2,
                          whiteText);

    const SDL_Rect labelField{dialog.x + 170, dialog.y + 94, 300, 42};
    const SDL_Rect valueField{dialog.x + 170, dialog.y + 158, 300, 42};
    drawPixelTextAt("LABEL", dialog.x + 45, labelField.y + 13, 1, darkText);
    drawPixelTextAt(definition->valueCaption,
                    dialog.x + 45,
                    valueField.y + 13,
                    1,
                    darkText);

    drawFilledRectangle(labelField,
                        SDL_Color{255, 255, 255, 255},
                        propertiesDialog.activeField == 0
                        ? selectionColor
                        : borderColor,
                        propertiesDialog.activeField == 0 ? 2 : 1);
    drawFilledRectangle(valueField,
                        SDL_Color{255, 255, 255, 255},
                        propertiesDialog.activeField == 1
                        ? selectionColor
                        : borderColor,
                        propertiesDialog.activeField == 1 ? 2 : 1);

    drawPixelTextAt(propertiesDialog.labelDraft +
                    (propertiesDialog.activeField == 0 ? "_" : ""),
                    labelField.x + 10,
                    labelField.y + 13,
                    1,
                    darkText);
    drawPixelTextAt(propertiesDialog.valueDraft +
                    (propertiesDialog.activeField == 1 ? "_" : ""),
                    valueField.x + 10,
                    valueField.y + 13,
                    1,
                    darkText);

    drawPixelTextAt("ROTATION: " + std::to_string(component->rotationDegrees()) +
                    " DEGREES",
                    dialog.x + 45,
                    dialog.y + 230,
                    1,
                    darkText);
    drawPixelTextAt("PIN COUNT: " +
                    std::to_string(definition->pins.size()),
                    dialog.x + 45,
                    dialog.y + 262,
                    1,
                    darkText);
    drawPixelTextAt("TAB: NEXT FIELD   ENTER: SAVE   ESC: CANCEL",
                    dialog.x + 45,
                    dialog.y + 292,
                    1,
                    darkText);

    const SDL_Rect saveButton{dialog.x + 275, dialog.y + 320, 95, 42};
    const SDL_Rect cancelButton{dialog.x + 380, dialog.y + 320, 95, 42};
    drawFilledRectangle(saveButton, buttonColor, borderColor, 1);
    drawFilledRectangle(cancelButton,
                        SDL_Color{110, 120, 125, 255},
                        borderColor,
                        1);
    drawPixelTextCentered("SAVE", saveButton, 1, whiteText);
    drawPixelTextCentered("CANCEL", cancelButton, 1, whiteText);
}

void ProteusApplication::renderCustomSizeDialog()
{
    boxRGBA(renderer, 0, 0, LOGICAL_WIDTH, LOGICAL_HEIGHT, 0, 20, 30, 150);
    const SDL_Rect dialog{430, 215, 500, 340};
    drawFilledRectangle(dialog,
                        SDL_Color{235, 244, 248, 255},
                        borderColor,
                        3);

    const SDL_Rect heading{dialog.x, dialog.y, dialog.w, 58};
    drawFilledRectangle(heading, titleColor, borderColor, 2);
    drawPixelTextCentered("CUSTOM CANVAS SIZE", heading, 2, whiteText);

    const SDL_Rect widthField{dialog.x + 175, dialog.y + 92, 270, 42};
    const SDL_Rect heightField{dialog.x + 175, dialog.y + 154, 270, 42};
    drawPixelTextAt("WIDTH (MM)", dialog.x + 35, widthField.y + 13, 1, darkText);
    drawPixelTextAt("HEIGHT (MM)", dialog.x + 35, heightField.y + 13, 1, darkText);

    drawFilledRectangle(widthField,
                        SDL_Color{255, 255, 255, 255},
                        customSizeDialog.activeField == 0
                        ? selectionColor
                        : borderColor,
                        customSizeDialog.activeField == 0 ? 2 : 1);
    drawFilledRectangle(heightField,
                        SDL_Color{255, 255, 255, 255},
                        customSizeDialog.activeField == 1
                        ? selectionColor
                        : borderColor,
                        customSizeDialog.activeField == 1 ? 2 : 1);

    drawPixelTextAt(customSizeDialog.widthDraft +
                    (customSizeDialog.activeField == 0 ? "_" : ""),
                    widthField.x + 10,
                    widthField.y + 13,
                    1,
                    darkText);
    drawPixelTextAt(customSizeDialog.heightDraft +
                    (customSizeDialog.activeField == 1 ? "_" : ""),
                    heightField.x + 10,
                    heightField.y + 13,
                    1,
                    darkText);

    if (!customSizeDialog.validationMessage.empty())
    {
        drawPixelTextCentered(customSizeDialog.validationMessage,
                              {dialog.x + 25, dialog.y + 214,
                               dialog.w - 50, 30},
                              1,
                              SDL_Color{170, 30, 30, 255});
    }
    else
    {
        drawPixelTextCentered("VALID RANGE: 100 TO 2000 MM",
                              {dialog.x + 25, dialog.y + 214,
                               dialog.w - 50, 30},
                              1,
                              darkText);
    }

    const SDL_Rect createButton{dialog.x + 235, dialog.y + 270, 100, 42};
    const SDL_Rect cancelButton{dialog.x + 345, dialog.y + 270, 100, 42};
    drawFilledRectangle(createButton, buttonColor, borderColor, 1);
    drawFilledRectangle(cancelButton,
                        SDL_Color{110, 120, 125, 255},
                        borderColor,
                        1);
    drawPixelTextCentered("CREATE", createButton, 1, whiteText);
    drawPixelTextCentered("CANCEL", cancelButton, 1, whiteText);
}

void ProteusApplication::cleanUp()
{
    clearTextCache();

    if (renderer != nullptr)
    {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window != nullptr)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}
