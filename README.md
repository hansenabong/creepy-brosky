# Creepy Brosky

A small Unreal Engine 5 horror game prototype built with C++ and Blueprints. The project is currently focused on learning Unreal Engine gameplay systems step by step while building a playable first-person horror experience.

## Current Features

- First-person exploration
- Interactable doors
- Contextual `[E]` interaction prompt
- Smooth door opening and closing
- Doors can open away from the player
- Door obstruction safety to avoid pushing through the player
- C++ gameplay logic exposed to Blueprint for configuration

## Controls

| Action | Input |
|---|---|
| Move | WASD |
| Look | Mouse |
| Interact | E |

## Tech Stack

- Unreal Engine 5
- C++
- Blueprints
- Visual Studio

## Project Status

Early prototype.

The current focus is on building and understanding one gameplay system at a time rather than rushing toward a large feature set.

## Planned Next Steps

- Locked door state
- Key pickup and unlocking
- Flashlight
- Fuse / power interaction
- Basic environmental horror events
- Simple enemy AI
- Death and restart flow
- Main menu
- Package a playable Windows build

## Development Approach

This project is also a learning project for Unreal Engine and C++. Features are being added incrementally so each system can be understood before expanding it.

The current door system separates player interaction from door-specific behaviour:

```text
Player Interaction
        ↓
Line Trace
        ↓
Interactable Door
        ↓
Toggle Door
        ↓
Smooth Door Animation
```

As development continues, the interaction system will be expanded to support additional objects such as keys, flashlights, switches, and other horror-game interactions.

## Playable escape demo

Open `Content/CreepyBrosky/Maps/Lvl_EscapeDemo` and press Play. The project now opens this map by default.

Find the brass key in the storage room, use E to unlock the service exit, and walk through it to finish. R restarts the run. Existing movement, mouse look and sprint remain available.

This demo builds on the original `bLocked` and `bHasKey` work. It adds a real key pickup, objective HUD, gated completion and replay. It is an editor-playable prototype, not a packaged release. See [Docs/DemoLesson.md](Docs/DemoLesson.md) for ownership, execution flow, manual checks and the next exercise.