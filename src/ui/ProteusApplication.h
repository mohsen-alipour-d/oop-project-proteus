#pragma once

#include "ComponentModel.h"
#include "../integration/backend_adapter.h"

#include <SDL2/SDL.h>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

enum class Page
{
    MainMenu,
    NewProjectMenu,
    DesignWorkspace
};

enum class CanvasPreset
{
    None,
    A4,
    A3,
    Custom
};

enum class ButtonType
{
    PageTitle,
    NewProject,
    OpenProject,
    RecentProjects,
    Exit,
    A4,
    A3,
    Customize
};

enum class ToolbarAction
{
    Select,
    Rotate,
    MirrorHorizontal,
    MirrorVertical,
    Delete,
    Save,
    Undo,
    Redo,
    Drc,
    RunPause,
    Stop,
    ResetView
};

enum class MouseOperation
{
    None,
    DragComponents,
    BoxSelect,
    Pan
};

class Button
{
public:
    Button(int x,
           int y,
           int width,
           int height,
           const std::string& buttonText,
           ButtonType buttonType,
           bool isClickable = true);

    bool contains(int mouseX, int mouseY) const;
    void updateHover(int mouseX, int mouseY);

    const SDL_Rect& bounds() const;
    const std::string& text() const;
    ButtonType type() const;
    bool isHovered() const;

private:
    SDL_Rect buttonBounds{};
    std::string buttonText;
    ButtonType buttonType;
    bool clickable;
    bool hovered = false;
};

class ProteusApplication
{
public:
    ProteusApplication() = default;
    ~ProteusApplication();

    bool initialize();
    void run();

private:
    static constexpr int LOGICAL_WIDTH = 1360;
    static constexpr int LOGICAL_HEIGHT = 800;
    static constexpr int TOOLBAR_HEIGHT = 58;
    static constexpr int STATUS_BAR_HEIGHT = 42;
    static constexpr int LEFT_PANEL_WIDTH = 274;
    static constexpr int RIGHT_PANEL_WIDTH = 274;
    static constexpr int GRID_SPACING = 20;
    static constexpr int MAX_LIBRARY_ROWS = 4;
    static constexpr int MAX_ACTIVE_ROWS = 3;

    struct ComponentRow
    {
        SDL_Rect bounds{};
        SDL_Rect removeBounds{};
        int definitionId = -1;
    };

    struct PropertiesDialog
    {
        bool open = false;
        int componentId = -1;
        int activeField = 0;
        std::string labelDraft;
        std::string valueDraft;
    };

    struct CustomSizeDialog
    {
        bool open = false;
        int activeField = 0;
        std::string widthDraft = "500";
        std::string heightDraft = "350";
        std::string validationMessage;
    };

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool running = true;

    Page currentPage = Page::MainMenu;
    CanvasPreset selectedPreset = CanvasPreset::None;
    int customCanvasWidthMillimeters = 500;
    int customCanvasHeightMillimeters = 350;

    std::vector<Button> menuButtons;
    std::unordered_map<std::string, SDL_Texture*> textTextureCache;

    ComponentLibrary componentLibrary;
    BackendAdapter backend;
    std::vector<ComponentInstance> components;
    std::vector<int> activeDefinitionIds;
    std::unordered_set<int> selectedComponentIds;
    std::unordered_map<int, int> nextLabelNumber;
    int nextComponentId = 1;
    std::optional<PinHandle> pendingWireStart;
    int selectedWireId = -1;
    bool simulationRunning = false;
    Uint64 lastFrameTicks = 0;
    std::string statusMessage = "READY";

    std::string selectedCategory = "ALL";
    std::string searchText;
    bool searchFocused = false;
    int libraryScrollOffset = 0;
    int activeScrollOffset = 0;
    int previewDefinitionId = 0;
    int placementDefinitionId = -1;

    std::vector<std::pair<SDL_Rect, std::string>> categoryRows;
    std::vector<ComponentRow> libraryRows;
    std::vector<ComponentRow> activeRows;
    std::vector<std::pair<SDL_Rect, ToolbarAction>> toolbarButtons;
    SDL_Rect searchBox{};
    SDL_Rect addToActiveButton{};
    SDL_Rect editPropertiesButton{};

    MouseOperation mouseOperation = MouseOperation::None;
    bool spaceHeld = false;
    int lastMouseX = -1;
    int lastMouseY = -1;
    WorldPoint canvasMouseWorld{};
    bool mouseInsideCanvas = false;

    WorldPoint dragStartWorld{};
    std::unordered_map<int, WorldPoint> originalDragPositions;
    SDL_Point selectionStartScreen{};
    SDL_Point selectionCurrentScreen{};
    SDL_Point panStartScreen{};
    double panStartX = 0.0;
    double panStartY = 0.0;

    double zoom = 1.0;
    double panX = 32.0;
    double panY = 32.0;

    bool contextMenuOpen = false;
    SDL_Point contextMenuPosition{};
    PropertiesDialog propertiesDialog;
    CustomSizeDialog customSizeDialog;

    const SDL_Color backgroundColor{131, 199, 229, 255};
    const SDL_Color titleColor{14, 83, 109, 255};
    const SDL_Color buttonColor{27, 100, 133, 255};
    const SDL_Color hoverColor{36, 118, 153, 255};
    const SDL_Color borderColor{6, 59, 75, 255};
    const SDL_Color whiteText{255, 255, 255, 255};
    const SDL_Color darkText{20, 72, 91, 255};
    const SDL_Color workspaceBackground{112, 181, 212, 255};
    const SDL_Color panelColor{23, 91, 123, 255};
    const SDL_Color panelItemColor{34, 112, 148, 255};
    const SDL_Color selectedItemColor{57, 145, 178, 255};
    const SDL_Color canvasColor{237, 244, 247, 255};
    const SDL_Color gridColor{204, 217, 222, 255};
    const SDL_Color majorGridColor{160, 184, 193, 255};
    const SDL_Color selectionColor{255, 153, 0, 255};
    const SDL_Color pinColor{200, 45, 45, 255};

    SDL_Rect toolbarArea() const;
    SDL_Rect leftPanelArea() const;
    SDL_Rect rightPanelArea() const;
    SDL_Rect canvasArea() const;
    SDL_Rect statusBarArea() const;

    static bool pointInsideRectangle(int x, int y, const SDL_Rect& rectangle);
    static SDL_Rect normalizedRectangle(const SDL_Point& first,
                                        const SDL_Point& second);

    void showMainMenu();
    void showNewProjectMenu();
    void showDesignWorkspace(CanvasPreset preset);
    void startNewProject(CanvasPreset preset);
    bool loadProject(bool mostRecent);
    void reloadFrontendFromBackend();
    void rebuildLabelNumbers();
    void updateSimulation();

    void processEvents();
    SDL_Point logicalMousePosition(int windowX, int windowY) const;
    void handleMouseMotion(int mouseX, int mouseY);
    void handleMouseButtonDown(const SDL_MouseButtonEvent& event,
                               int mouseX,
                               int mouseY);
    void handleMouseButtonUp(const SDL_MouseButtonEvent& event,
                             int mouseX,
                             int mouseY);
    void handleMouseWheel(const SDL_MouseWheelEvent& event);
    void handleKeyDown(const SDL_KeyboardEvent& event);
    void handleKeyUp(const SDL_KeyboardEvent& event);
    void handleTextInput(const std::string& text);

    void handleMenuClick(int mouseX, int mouseY);
    void performMenuAction(ButtonType type);
    void handleWorkspaceLeftClick(int mouseX,
                                  int mouseY,
                                  int clickCount,
                                  SDL_Keymod modifiers);
    void handleWorkspaceRightClick(int mouseX, int mouseY);
    void finishCanvasOperation(int mouseX, int mouseY, SDL_Keymod modifiers);

    bool handleLeftPanelClick(int mouseX, int mouseY, int clickCount);
    bool handleToolbarClick(int mouseX, int mouseY);
    bool handleRightPanelClick(int mouseX, int mouseY);
    bool handleContextMenuClick(int mouseX, int mouseY);
    bool handlePropertiesDialogClick(int mouseX, int mouseY);
    bool handleCustomDialogClick(int mouseX, int mouseY);

    void startComponentDrag(int componentId, const WorldPoint& mouseWorld);
    void beginBoxSelection(int mouseX, int mouseY);
    void beginPan(int mouseX, int mouseY);
    void updateComponentDrag(const WorldPoint& mouseWorld);
    void applyBoxSelection(SDL_Keymod modifiers);

    void armComponentForPlacement(int definitionId);
    void placeComponent(int definitionId, const WorldPoint& worldPosition);
    void addDefinitionToActiveList(int definitionId);
    void removeDefinitionFromActiveList(int definitionId);
    bool isDefinitionActive(int definitionId) const;

    ComponentInstance* componentById(int id);
    const ComponentInstance* componentById(int id) const;
    int componentAt(const WorldPoint& point) const;
    void selectOnly(int componentId);
    void toggleSelection(int componentId);
    void clearSelection();
    void selectAllComponents();
    void deleteSelectedComponents();
    void rotateSelectedComponents();
    void mirrorSelectedComponentsHorizontally();
    void mirrorSelectedComponentsVertically();
    void saveProject();
    void undoProject();
    void redoProject();
    void runDesignRuleCheck();
    void toggleSimulation();
    void stopSimulation();
    void recordProjectChange();

    void openPropertiesDialog(int componentId);
    void closePropertiesDialog(bool saveChanges);
    void openCustomSizeDialog();
    void closeCustomSizeDialog(bool createProject);
    std::string& activeDialogText();
    void removeLastUtf8Character(std::string& text);

    WorldPoint screenToWorld(int screenX, int screenY) const;
    SDL_Point worldToScreen(const WorldPoint& point) const;
    double snapCoordinate(double coordinate) const;
    WorldPoint snapPoint(const WorldPoint& point) const;
    SDL_Rect componentScreenBounds(const ComponentInstance& component,
                                   const ComponentDefinition& definition) const;
    void zoomAt(int screenX, int screenY, double factor);
    void resetView();

    std::string presetName() const;
    std::string presetDimensions() const;
    std::string createTextureKey(const std::string& text,
                                 const SDL_Color& color) const;
    SDL_Texture* textTexture(const std::string& text,
                             const SDL_Color& color);
    void clearTextCache();

    void drawPixelTextAt(const std::string& text,
                         int x,
                         int y,
                         int scale,
                         const SDL_Color& color);
    void drawPixelTextCentered(const std::string& text,
                               const SDL_Rect& area,
                               int preferredScale,
                               const SDL_Color& color);
    void drawFilledRectangle(const SDL_Rect& rectangle,
                             const SDL_Color& fill,
                             const SDL_Color& border,
                             int borderThickness = 2);
    void drawOutlineRectangle(const SDL_Rect& rectangle,
                              const SDL_Color& color,
                              int borderThickness = 1);

    void render();
    void renderMenuPage();
    void renderDesignWorkspace();
    void renderTopToolbar(const SDL_Rect& toolbar);
    void renderLeftComponentPanel(const SDL_Rect& leftPanel);
    void renderRightPropertiesPanel(const SDL_Rect& rightPanel);
    void renderCanvas(const SDL_Rect& canvas);
    void renderWires();
    void renderPendingWire();
    void renderGrid(const SDL_Rect& canvas);
    void renderStatusBar(const SDL_Rect& statusBar);
    void renderComponent(const ComponentInstance& component);
    void renderPlacementPreview();
    void renderDefinitionPreview(const ComponentDefinition& definition,
                                 const SDL_Rect& previewArea);
    void drawSymbol(const ComponentDefinition& definition,
                    const std::function<SDL_Point(const WorldPoint&)>& mapper,
                    const SDL_Color& symbolColor,
                    bool drawPins);
    void renderSelectionRectangle();
    void renderContextMenu();
    void renderPropertiesDialog();
    void renderCustomSizeDialog();

    void rebuildLeftPanelHitAreas();
    void rebuildToolbarHitAreas();
    void cleanUp();
};
