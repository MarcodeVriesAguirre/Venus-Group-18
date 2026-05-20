# Pynq

C source for the robot, built against libpynq.

## Layout
- drivers/        shared sensor drivers (no main())
- distance_test/  standalone VL53L0X test
- color_test/     standalone TCS3472 test
- robot/          main robot program

## Build (on the PYNQ, one-time setup)
    cd ~/libpynq-5EID0-2023-v0.3.0/applications
    ln -sf ~/Venus-Group-18/Pynq/distance_test .
    ln -sf ~/Venus-Group-18/Pynq/color_test .
    ln -sf ~/Venus-Group-18/Pynq/robot .

## Build any app
    cd ~/libpynq-5EID0-2023-v0.3.0/applications/robot
    make
    sudo ./main
