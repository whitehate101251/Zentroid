# Zentroid Keymap Editor

Visual drag-and-drop keymap editor for Zentroid.

## Features

- ✅ Visual canvas with device screenshot background
- ✅ Drag-and-drop node placement
- ✅ Support for 5 mapping types:
  - Click (KMT_CLICK)
  - Click Twice (KMT_CLICK_TWICE)
  - Drag (KMT_DRAG)
  - WASD Steering Wheel (KMT_STEER_WHEEL)
  - Multi-click (KMT_CLICK_MULTI) [TODO]
- ✅ JSON import/export (compatible with existing format)
- ✅ Profile management
- ✅ Zoom in/out
- ⏳ Live preview (TODO)
- ⏳ Undo/Redo (TODO)

## Usage

1. Launch editor from main window
2. Select tool from toolbar (Click/Drag/WASD)
3. Click on canvas to place node
4. Assign key to node
5. Save to keymap/ directory
6. Apply keymap in main window

## Shortcuts

- `S` - Select mode
- `C` - Add Click node
- `D` - Add Drag node
- `W` - Add WASD node
- `Del` - Delete selected
- `Ctrl+S` - Save
- `Ctrl+O` - Open
- `Ctrl+N` - New
- `+/-` - Zoom

## Architecture

```
KeymapEditorDialog (main window)
├── QGraphicsView (canvas)
│   └── QGraphicsScene
│       ├── Background (screenshot)
│       └── KeyNodes (draggable)
├── QToolBar (tools)
└── QStatusBar (info)

KeyNode (base class)
├── ClickNode
├── ClickTwiceNode
├── DragNode
└── SteerWheelNode
```

## File Format

Maintains compatibility with Zentroid's JSON format:

```json
{
  "switchKey": "Key_QuoteLeft",
  "keyMapNodes": [
    {
      "comment": "Fire",
      "type": "KMT_CLICK",
      "key": "Key_F",
      "pos": {"x": 0.8, "y": 0.7},
      "switchMap": false
    }
  ]
}
```

## Development Status

Phase 1: Foundation ✅ COMPLETE
- Basic editor window
- Canvas with screenshot
- Node classes
- JSON serialization

Phase 2: Node System (IN PROGRESS)
- Node creation
- Node editing
- Key assignment dialog

Phase 3: Data Layer (TODO)
- Full JSON parsing
- Validation
- Error handling

Phase 4: Profile Management (TODO)
- Multi-profile support
- Import/Export

Phase 5: Polish & UX (TODO)
- Visual refinements
- Properties panel
- Better icons
