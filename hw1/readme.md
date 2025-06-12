# Interpolation and Fitting Visualization Tool

![Application Screenshot](screenshot.png)

An interactive Qt5 application for visualizing and comparing different interpolation and fitting algorithms in real-time.

## Features

- **Multiple Algorithms**:
  - Polynomial interpolation
  - Gaussian interpolation
  - Least squares fitting
  - Ridge regression fitting
  
- **Interactive Point Management**:
  - Left-click to add points
  - Drag points with left mouse button
  - Right-click to delete points
    - Right-click on point: Delete specific point
    - Right-click on empty space: Delete last point
  
- **Real-time Visualization**:
  - Curves update instantly as points are moved or added
  - Multiple curves can be displayed simultaneously
  - Toggle algorithm visibility with checkboxes
  
- **Parameter Control**:
  - Adjust polynomial degree (1-10)
  - Modify Gaussian sigma (1-100)
  - Tune ridge regression lambda (0.01-1.00)
  
- **Coordinate Systems**:
  - Screen coordinates (top-left origin)
  - Mathematical coordinates (bottom-left origin)
  - Hover over points to see both coordinate systems

## Requirements

- **System**:
  - Linux, Windows, or macOS
  - OpenGL compatible graphics
  
- **Dependencies**:
  - Qt5 (≥ 5.9)
  - OpenCV (≥ 3.4)
  - Eigen3
  - C++17 compatible compiler

## Build Instructions

```bash
# Clone repository
git clone https://github.com/Mavericks2019/games102.git
cd interpolation-tool

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make

# Run application
./InterpolationTool