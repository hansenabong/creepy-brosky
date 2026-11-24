# CREEPY BROSKY

> *A story too absurd to believe. A nightmare too real to escape.*

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.8-black?logo=unrealengine)
![C++](https://img.shields.io/badge/Language-C%2B%2B-blue?logo=cplusplus)
![Platform](https://img.shields.io/badge/Platform-Windows-blue)
![Status](https://img.shields.io/badge/Status-In%20Development-orange)

## About the Game

**Creepy Brosky** is an independently developed first-person psychological survival horror game inspired by Indonesian urban legends, Javanese folklore, and the fear of the unknown.

Combining supernatural encounters, environmental storytelling, and psychological tension, the game explores the uncertainty of memory, identity, and perception.

## Synopsis

*749 kilometers from home.*

A young man named Brosky wakes up in the back of a pickup truck with no recollection of how he got there.

Somehow, he has travelled from Bekasi to the remote surroundings of Mount Kelud, East Java.

Lost in an unfamiliar forest, Brosky must search for a way home. But as darkness falls, strange occurrences begin to challenge his understanding of reality.

A mysterious voice offers guidance. Familiar things begin to feel unfamiliar. What initially appears to be a search for safety slowly becomes something far more disturbing.

**Something is wrong with this place.**

*And perhaps, something is wrong with him.*

## Creative Direction

The game aims to deliver a horror experience that gradually evolves from immediate supernatural threats into lingering psychological dread.

- First-person psychological survival horror
- Indonesian folklore-inspired supernatural encounters
- Atmospheric exploration and environmental storytelling
- Unreliable narration
- Survival-oriented gameplay
- Tension-driven enemy encounters

These elements represent the intended creative direction and are not all implemented in the current prototype.

## Current Prototype

The current development milestone focuses on establishing a functional gameplay foundation before expanding into the full narrative experience.

### Gameplay Systems

- First-person movement and exploration
- Component-based player interaction system
- Visibility-aware C++ line tracing
- Interactive doors with smooth animation
- Directional door opening and obstruction handling
- Collectible keys and locked-door mechanics
- Objective-based escape gameplay
- Contextual interaction prompts and HUD feedback
- Centralized win, loss, and restart states
- Timed enemy capture mechanics

### Enemy AI

The project includes an evolving C++ enemy AI implementation featuring:

- Patrol, Chase, and Search states
- Unreal Engine NavMesh navigation
- Line-of-sight detection
- Last-known player position tracking
- Pathfinding failure handling
- Configurable detection and capture conditions

Enemy navigation integration and gameplay validation remain in development.

## Technical Architecture

The project follows a modular approach that separates gameplay responsibilities across Unreal Engine C++ classes and Blueprint assets.

| System | Responsibility |
|---|---|
| Interaction Component | Player input, line tracing, and object interaction |
| Interactable Door | Door animation, locking, and obstruction handling |
| Key Pickup | Item collection and player key state |
| Exit Zone | Objective validation and escape completion |
| GameMode | Gameplay state management |
| HUD | Objectives, interaction prompts, and feedback |
| Enemy Character | Enemy movement and capture behaviour |
| AI Controller | Enemy perception, navigation, and decision-making |

Core gameplay logic is implemented in C++, while Blueprints are used for configuration, asset integration, and presentation.

## Technology Stack

- Unreal Engine 5.8
- C++
- Unreal Engine Blueprints
- Enhanced Input System
- Unreal Navigation System
- Visual Studio
- Git and GitHub

## Controls

| Action | Input |
|---|---|
| Movement | WASD |
| Camera | Mouse |
| Interact | E |
| Restart | R |

## Development Roadmap

- [x] First-person gameplay foundation
- [x] Player interaction system
- [x] Interactive doors and key mechanics
- [x] Escape objectives
- [x] Win, loss, and restart flow
- [x] Enemy capture system
- [x] Internal Windows Development build
- [ ] Complete enemy AI integration and validation
- [ ] Atmospheric level design
- [ ] Environmental audio and sound design
- [ ] Main menu and settings
- [ ] External playtesting
- [ ] Narrative gameplay expansion

## Development Philosophy

Creepy Brosky is both an independent game project and a practical learning environment for exploring game engineering.

Development focuses on understanding gameplay architecture, C++ programming, real-time systems, artificial intelligence, debugging, and the relationship between native code and Unreal Engine's Blueprint system.

AI-assisted development tools are used alongside manual testing, code review, and ongoing technical learning.

## Repository Notice

This repository contains the public technical implementation of Creepy Brosky.

Internal narrative documentation, story development, and selected prototype assets are maintained privately. As a result, some assets or dependencies may require replacement before the public project can be built or played as intended.

The game is currently in active development and does not represent a finished commercial release.

---

*Not every road leads home.*
