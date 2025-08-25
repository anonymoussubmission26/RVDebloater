# RVDebloater Artifact

Welcome to the artifact for the USENIX 2026 submission of our paper, titled **"RVDebloater: Mode-based Adaptive Firmware Debloating for Robotic Vehicles"**.

## Instructions

1. **Clone the Repository**

    - Create an RVDebloater Project Directory and set the environment variable ```$RVD_PROJ```

    ```bash
    mkdir rvd-project
    echo 'export RVD_PROJ=${HOME}/rvd-project' >> ${HOME}/.bashrc
    source ${HOME}/.bashrc
    ```
    *Change ${HOME}/rvd-project to the correct path and name if you use a different path or name*

    - Clone the RVDebloater repository:

    ```bash
    cd ${RVD_PROJ}
    git clone git@github.com:anonymoussubmission26/RVDebloater.git
    ```

2. **Pre-requisites**

    - If `go` is already installed check the version
        - check version using 
            ```bash
            /usr/local/go/bin/go version
            ```
        - If version is 1.17 or higher, skip to the next step, otherwise remove it with the following
            ```bash
            /usr/local/go/bin/go env GOPATH
            ```
            and run ```sudo rm -rf [directory]``` on the resulting directory, for example:
            ```bash
            sudo rm -rf ${HOME}/go
            ```
            lastly run
            ```bash
            sudo rm -rf /usr/local/go
            ```

    - Run the prereq script to install dependencies and set environment variables:

        ```bash
        cd ${RVD_PROJ}
        ./RVDebloater/scripts/pre-req.sh
        ```

        The script does the following:
        - Installs some dependencies for `SVF` and `gllvm`
        - Installs our version of `gllvm` (`ap-gllvm`) using go
        - Clones `SVF` and `ArduPilot` repositories to `rvd-project` directory
        - Builds and configures `SVF` 

3. **Instrumented Simulation**

    The `run-simulation.sh` script does the following:
    - Builds ArduPilot copter using our modified version of gllvm
    - Extracts the whole program bitcode file
    - Runs instrumentation on the bitcode file 
        - *Note: This is not yet implemented, add implementation to the `instrument-ap.sh` script*
    - Recompiles the bitcode back to executable
    - Runs the ArduPilot SITL simulation using the instrumented copter executable
    - The generated files including final executable and bitcode file are stored in `${AP_BIN}` (e.g., `rvd-project/ardupilot/build/sitl/bin`)
      
    **Run with**:
    ```bash
    cd ${RVD_PROJ}
    RVDebloater/scripts/run-simulation.sh
    ```
4. **Running Vehicle In Simulation**

    *The following instructions are for copter since it is the default vehicle*  

    - Once the simulation has loaded, wait for the following 2 lines to appear in the console  
        ```AP: EKF3 IMU0 is using GPS```  
        ```AP: EKF3 IMU1 is using GPS```

    - Then run the following 3 commands in the terminal:
        - ```mode guided```
        - ```arm throttle```
        - ```takeoff 10```

        *each of these commands should produce a message in the console indicating that the command was accepted, e.g.* ```Got COMMAND_ACK: DO_SET_MODE: ACCEPTED``` *after* ```mode guided``` 

        For plane the commands would be:
        - ```qloiter```
        - ```arm throttle```
        - ```rc 3 1500```
        - ```rc 3 1700```
        - ```qhover```

    - Then try repositioning the vehicle
        - Right-click anywhere on the mini map
        - Select "Fly To"
        - Press "OK" on the Altitude pop up window after entering the desired value

    - The vehicle should begin moving to the selected location
