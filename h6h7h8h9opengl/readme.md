# OpenGL Mesh Viewer

This OpenGL-based mesh viewer application allows you to visualize 3D models with various rendering techniques including wireframe mode, texture mapping, curvature visualization, and lighting effects. The application is designed to run on Windows Subsystem for Linux (WSL) with X11 forwarding.

## Features

- Multiple rendering modes:
  - Blinn-Phong shading with specular highlights
  - Curvature visualization with color mapping
  - Texture mapping support
  - Wireframe rendering
- Interactive camera controls
- Real-time lighting adjustments
- Support for common 3D file formats via OpenMesh

## Prerequisites

1. **Windows 10/11** with WSL 2 enabled (recommended)
2. **Ubuntu 20.04/22.04** installed via Microsoft Store
3. **VcXsrv** for X11 forwarding ([download here](https://sourceforge.net/projects/vcxsrv/))

## Setup Instructions

### 1. Install Required Packages
Open your WSL terminal and run:
```bash
sudo apt update && sudo apt upgrade -y
sudo apt install -y build-essential cmake libgl1-mesa-dev libglew-dev libglfw3-dev libxinerama-dev libxcursor-dev libxi-dev