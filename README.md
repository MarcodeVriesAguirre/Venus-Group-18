# Pynq

Project Venus is an autonomous PYNQ two robot system that explores an unknown arena on its
own. It has to find the cubes scattered around, drive up to each one, identify it
by size (small / large) and color, and avoid a cardboard "mountain", all while
mapping its position on a grid. This folder holds the robot code plus a test
program for each sensor. Every folder builds to a
`main` and can be executed on the board as follows:

```sh
cd <folder>
make
./main
```

## Folders

- **robot/** — the full robot. `algorithm.c` is the main code that runs on the robot: it drives up to an
  object with the distance sensor, sweeps to measure cube width (small / large /
  mountain), reads its color, and tracks position on the grid. The rest are
  helpers: `movement.c` (motors/turning), `communication.c` (UART),
  `shared.h` (shared data)
- **color_test/** — reads the color sensor and classifies the surface (`main.c`).
- **distance_test/** — prints distance from the VL53L0X ToF sensor (`main.c`).
- **Infrared/** — prints the IR sensor level (0/1) for edge detection (`main.c`).
- **temperature/** — reads the thermistor and converts it to °C (`main.c`).
- **Comunication/** — PC-side scripts to talk to the robot (not a make/run folder).
