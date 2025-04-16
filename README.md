# 3D ASCII Minecraft Clone

A terminal-based 3D ASCII art version of Minecraft, written in C. This project creates a simple but engaging voxel-based world where you can move around, place and remove blocks, all rendered in ASCII characters.

## Features

- 3D voxel-based world with block placement and removal
- First-person perspective with mouse look controls
- Real-time ASCII rendering
- Smooth movement and collision detection
- Block highlighting and interaction
- Terminal-based interface

## Prerequisites

- Unix-like operating system (Linux, macOS)
- C compiler (gcc)
- Terminal with support for ANSI escape codes
- Make build system

## Building

To compile the project, simply run:

```bash
make
```

This will create an executable named `minecraft`.

## Running

To start the game:

```bash
./minecraft
```

## Controls

### Movement
- `W` - Look up
- `S` - Look down
- `A` - Look left
- `D` - Look right
- `I` - Move forward
- `K` - Move backward
- `J` - Strafe left
- `L` - Strafe right

### Block Interaction
- `Space` - Place block
- `X` - Remove block
- `Q` - Quit game

## World Structure

The world is divided into a 3D grid with the following dimensions:
- Width: 20 blocks
- Height: 20 blocks
- Depth: 10 blocks

The ground level is automatically filled with blocks (represented by '@' characters).

## Technical Details

### Rendering
- Display resolution: 900x180 characters
- Field of view: 60 degrees horizontal, 40 degrees vertical
- Ray casting algorithm for 3D rendering
- ASCII characters used for different block types:
  - '@' - Solid block
  - 'o' - Highlighted block
  - '-' - Block edge
  - ' ' - Air/empty space

### Physics
- Simple collision detection
- Player eye level: 1.5 blocks
- Movement speed: 0.3 blocks per frame
- Rotation speed: 0.1 radians per frame

## Implementation Notes

The game uses several key techniques:
- Ray casting for 3D rendering
- Vector mathematics for movement and view calculations
- Terminal manipulation for real-time display
- Non-blocking input handling

## Limitations

- Limited to terminal-based graphics
- Fixed world size
- Basic physics
- Single block type
- No texture support

## Contributing

Feel free to fork this project and submit pull requests with improvements. Some potential areas for enhancement:
- Multiple block types
- World saving/loading
- More complex world generation
- Improved physics
- Multiplayer support

## License

This project is open source and available under the MIT License.

## Acknowledgments

- Inspired by the original Minecraft game
- Uses standard C libraries for terminal manipulation and mathematics
- Built as a demonstration of 3D rendering in ASCII art