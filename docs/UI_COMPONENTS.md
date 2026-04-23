# UI Component Library

A complete suite of reusable UI components for the game engine. All components follow a consistent pattern with `initialize()`, `render()`, and input handling methods.

---

## UIButton

**File**: `src/ui/UIButton.h` / `UIButton.cpp`

### What It Is
A clickable button with visual feedback for hover and press states.

### Common Uses
- Menu buttons (Play, Settings, Quit)
- Action buttons in HUDs
- Dialog buttons (OK, Cancel, Confirm)
- Toolbar buttons
- Any clickable UI element

### How It Works
- **Visual States**: Three color states (normal, hover, pressed) that change based on mouse interaction
- **Click Detection**: Checks if mouse is within button bounds on press, triggers callback only if released while still hovering
- **Text Centering**: Automatically centers text label within the button bounds
- **Customization**: Colors can be customized via `setColors()`, text via `setText()`

### Usage Example
```cpp
UIButton playButton;
playButton.initialize(100, 200, 200, 50, "Play", font);
playButton.setCallback([]() {
    std::cout << "Play clicked!" << std::endl;
    gameState = GameState::Playing;
});

// In event loop:
playButton.handleMousePress(mousePos);
playButton.handleMouseMove(mousePos);
playButton.handleMouseRelease();

// In render:
playButton.render(renderer);
```

---

## UICheckbox

**File**: `src/ui/UICheckbox.h` / `UICheckbox.cpp`

### What It Is
A toggle checkbox with a checkmark (✓) and label text.

### Common Uses
- Enable/disable options (fullscreen, VSync, show debug info)
- Boolean settings toggles
- Selection lists (multi-select)
- Feature flags in settings menus

### How It Works
- **Toggle Behavior**: Clicking checkbox or label toggles `isChecked` state
- **Visual Feedback**: Shows/hides checkmark character based on state
- **Clickable Area**: Extends clickable region to include label for easier interaction
- **Callback**: Triggers `onToggle(bool)` callback when state changes

### Usage Example
```cpp
UICheckbox fullscreenCheckbox;
fullscreenCheckbox.initialize(100, 100, 20, "Fullscreen Mode", font);
fullscreenCheckbox.setCallback([](bool checked) {
    window.setFullscreen(checked);
});

// In event loop:
fullscreenCheckbox.handleMousePress(mousePos);

// In render:
fullscreenCheckbox.render(renderer);
```

---

## UIProgressBar

**File**: `src/ui/UIProgressBar.h` / `UIProgressBar.cpp`

### What It Is
A horizontal bar that visually represents a value between min and max (health, loading, progress).

### Common Uses
- Health/HP bars
- Loading/progress indicators
- Experience bars
- Stamina/mana meters
- Download/upload progress
- Timers or countdowns

### How It Works
- **Value Mapping**: Converts current value to percentage (0.0 to 1.0) and scales fill bar width accordingly
- **Automatic Updates**: Call `setValue()` to update, which automatically recalculates fill width and text
- **Value Display**: Shows "current / max" text above bar (can be disabled with `setShowText(false)`)
- **Customization**: Background, fill, and text colors can be customized

### Usage Example
```cpp
UIProgressBar healthBar;
healthBar.initialize(100, 100, 300, 30, 0.0f, 100.0f, 75.0f, font);

// Update dynamically:
healthBar.setValue(50.0f);  // Fills to 50%

// Customize colors:
healthBar.setColors(
    sf::Color(40, 40, 40),    // Background (dark gray)
    sf::Color(255, 50, 50),   // Fill (red for health)
    sf::Color::White           // Text
);

// In render:
healthBar.render(renderer);
```

---

## UIToggle

**File**: `src/ui/UIToggle.h` / `UIToggle.cpp`

### What It Is
A modern on/off toggle switch with a sliding thumb animation.

### Common Uses
- Modern UI settings (replaces checkboxes for on/off)
- Feature toggles
- Mode switches (day/night, online/offline)
- Quick enable/disable controls
- iOS/Android-style toggles

### How It Works
- **Sliding Thumb**: Circular thumb slides left (off) or right (on) within a pill-shaped track
- **Color Change**: Track color changes between off (gray) and on (green) states
- **Toggle Action**: Clicking track or label toggles state
- **Callback**: Triggers `onToggle(bool isOn)` when state changes

### Usage Example
```cpp
UIToggle vsyncToggle;
vsyncToggle.initialize(100, 100, 60, 30, "VSync", font, true);  // Start enabled
vsyncToggle.setCallback([](bool isOn) {
    window.setVSyncEnabled(isOn);
});

// In event loop:
vsyncToggle.handleMousePress(mousePos);

// In render:
vsyncToggle.render(renderer);
```

---

## UITextInput

**File**: `src/ui/UITextInput.h` / `UITextInput.cpp`

### What It Is
A text input field with keyboard handling, blinking cursor, and submit callback.

### Common Uses
- Player name entry
- Chat/message input
- Console commands
- Search fields
- Configuration text (server address, file paths)
- Any user text entry

### How It Works
- **Focus Management**: Click input box to focus, click elsewhere to unfocus
- **Keyboard Input**: Handles `TextEntered` events for character input, `KeyPressed` for backspace/enter
- **Blinking Cursor**: Cursor blinks every 0.5 seconds when focused (call `updateCursorVisibility()` in game loop)
- **Max Length**: Enforces character limit via `setMaxLength()`
- **Submit Event**: Pressing Enter triggers `onSubmit(string)` callback with entered text
- **Validation**: Only accepts printable ASCII characters (32-126)

### Usage Example
```cpp
UITextInput nameInput;
nameInput.initialize(100, 100, 200, 30, "Player Name:", font);
nameInput.setMaxLength(20);
nameInput.setCallback([](const std::string& name) {
    std::cout << "Submitted: " << name << std::endl;
    player.setName(name);
});

// In event loop:
nameInput.handleMousePress(mousePos);

// For keyboard events:
if (event.is<sf::Event::KeyPressed>()) {
    nameInput.handleKeyPress(event.get<sf::Event::KeyPressed>());
}
if (event.is<sf::Event::TextEntered>()) {
    nameInput.handleTextEntered(event.get<sf::Event::TextEntered>());
}

// In update loop (for cursor blink):
nameInput.updateCursorVisibility(deltaTime);

// In render:
nameInput.render(renderer);
```

---

## UIDropdown

**File**: `src/ui/UIDropdown.h` / `UIDropdown.cpp`

### What It Is
A dropdown selection menu that expands to show a list of options when clicked.

### Common Uses
- Resolution selection
- Graphics quality presets
- Language selection
- Difficulty selection
- Any single-choice from multiple options
- ComboBox-style UI

### How It Works
- **Expandable List**: Click dropdown box to expand/collapse option list
- **Option Management**: Add options via `addOption()`, auto-selects first option
- **Hover Highlight**: Options highlight on mouse hover for visual feedback
- **Selection**: Clicking an option selects it, closes dropdown, triggers callback
- **Auto-Collapse**: Clicking outside dropdown closes it without selection
- **State Access**: Get selected value/index via `getSelectedValue()` / `getSelectedIndex()`

### Usage Example
```cpp
UIDropdown resolutionDropdown;
resolutionDropdown.initialize(100, 100, 200, 30, font);
resolutionDropdown.addOption("1920x1080");
resolutionDropdown.addOption("1280x720");
resolutionDropdown.addOption("800x600");
resolutionDropdown.setCallback([](int index, const std::string& value) {
    std::cout << "Selected: " << value << std::endl;
    applyResolution(value);
});

// In event loop:
resolutionDropdown.handleMousePress(mousePos);
resolutionDropdown.handleMouseMove(mousePos);

// Get current selection:
std::string currentRes = resolutionDropdown.getSelectedValue();

// In render:
resolutionDropdown.render(renderer);
```

---

## UIRadioButton

**File**: `src/ui/UIRadioButton.h` / `UIRadioButton.cpp`

### What It Is
A radio button for mutually exclusive selections within a group (only one can be selected at a time).

### Common Uses
- Difficulty selection (Easy/Medium/Hard)
- Character class selection
- Game mode selection
- Payment method selection
- Any single-choice UI where options are visible simultaneously

### How It Works
- **Group Management**: Static map tracks all radio buttons by group ID
- **Mutual Exclusion**: Selecting one button automatically deselects others in same group via `clearGroup()`
- **Visual States**: Outer circle always visible, inner circle (dot) shows when selected
- **Registration**: Buttons auto-register with their group on `initialize()`
- **Group Callback**: Set callback per group to respond to any selection change
- **Dynamic Groups**: Can change group membership via `setGroupId()`

### Usage Example
```cpp
UIRadioButton difficulty[3];
difficulty[0].initialize(100, 100, "Easy", 1, font);
difficulty[1].initialize(100, 140, "Medium", 1, font);
difficulty[2].initialize(100, 180, "Hard", 1, font);

// Set group callback (called when any button in group is selected):
UIRadioButton::setCallback(1, [](int groupId) {
    std::cout << "Difficulty changed in group " << groupId << std::endl;
});

// Manually select one:
difficulty[1].setSelected(true);  // Medium selected by default

// In event loop:
for (auto& radio : difficulty) {
    radio.handleMousePress(mousePos);
}

// In render:
for (auto& radio : difficulty) {
    radio.render(renderer);
}
```

---

## UITooltip

**File**: `src/ui/UITooltip.h` / `UITooltip.cpp`

### What It Is
A floating tooltip that appears near the cursor with fade in/out animations.

### Common Uses
- Button descriptions
- Item descriptions in inventory
- Help hints
- Context information
- Tutorial prompts
- Hover explanations for complex UI

### How It Works
- **Fade Animation**: Smoothly fades in/out over configurable duration (default 0.2s)
- **Auto-Sizing**: Automatically resizes background to fit text content
- **Smart Positioning**: Positions above/right of cursor, adjusts to avoid screen edges
- **Lifecycle**: Call `show()` to display, `hide()` to fade out, `update()` in game loop for animation
- **Dynamic Text**: Can change text while visible via `setText()`
- **Alpha Control**: Manages alpha separately from SFML colors to allow fade effect

### Usage Example
```cpp
UITooltip tooltip;
tooltip.initialize(font);
tooltip.setFadeDuration(0.3f);  // 300ms fade

// Show when hovering over a button:
if (button.isHovered) {
    tooltip.show("Click to start the game", mouseX, mouseY);
} else {
    tooltip.hide();
}

// In update loop (REQUIRED for fade animation):
tooltip.update(deltaTime);

// In render:
tooltip.render(renderer);
```

---

## UINumberInput

**File**: `src/ui/UINumberInput.h` / `UINumberInput.cpp`

### What It Is
A numeric input field with +/- buttons for incrementing/decrementing values, plus direct keyboard input.

### Common Uses
- Volume/brightness sliders (with precise control)
- Numeric settings (FOV, sensitivity)
- Quantity selectors (buy X items)
- Timer duration input
- Grid size configuration
- Any numeric value entry

### How It Works
- **Dual Input Methods**: 
  - Click +/- buttons to increment/decrement by step size
  - Click display box to type number directly
- **Step Size**: Configurable increment amount via `setStep()` (default 1.0)
- **Range Validation**: Enforces min/max bounds automatically
- **Keyboard Input**: Direct number entry with Enter to confirm, validates and clamps value
- **Visual Feedback**: Buttons change color on hover/press
- **Callback**: Triggers `onChange(float)` when value changes via any method

### Usage Example
```cpp
UINumberInput volumeInput;
volumeInput.initialize(100, 100, 150, 30, 0.0f, 100.0f, 50.0f, font);
volumeInput.setStep(5.0f);  // +/- 5 per click
volumeInput.setCallback([](float value) {
    audio.setVolume(value);
    std::cout << "Volume: " << value << std::endl;
});

// In event loop:
volumeInput.handleMousePress(mousePos);
volumeInput.handleMouseMove(mousePos);
volumeInput.handleMouseRelease();

// For keyboard events:
if (event.is<sf::Event::KeyPressed>()) {
    volumeInput.handleKeyPress(event.get<sf::Event::KeyPressed>());
}
if (event.is<sf::Event::TextEntered>()) {
    volumeInput.handleTextEntered(event.get<sf::Event::TextEntered>());
}

// In render:
volumeInput.render(renderer);
```

---

## Common Patterns

### Initialization
All components follow this pattern:
```cpp
Component component;
component.initialize(x, y, width, height, ..., font);
```

### Rendering
```cpp
component.render(renderer);
```

### Input Handling
```cpp
// Mouse events (all components):
component.handleMousePress(mousePos);
component.handleMouseMove(mousePos);
component.handleMouseRelease();

// Keyboard events (UITextInput, UINumberInput):
component.handleKeyPress(keyEvent);
component.handleTextEntered(textEvent);
```

### Callbacks
All interactive components support callbacks via `std::function`:
```cpp
component.setCallback([](parameters) {
    // Handle event
});
```

### State Access
Public member variables for easy state checking:
```cpp
bool initialized = false;  // All components
// Component-specific:
component.isChecked;       // UICheckbox
component.isOn;            // UIToggle
component.value;           // UIProgressBar, UINumberInput
component.isHovered;       // UIButton
```

---

## Design Principles

1. **Self-Contained**: Each component manages its own SFML objects (shapes, text)
2. **Consistent API**: Same method names across all components
3. **Callback-Driven**: Use callbacks for event handling instead of polling
4. **Memory Safe**: Destructors clean up dynamically allocated sf::Text objects
5. **Customizable**: Colors, sizes, and labels can be changed after initialization
6. **Reusable**: Components can be used anywhere in menus, HUDs, or debug tools
