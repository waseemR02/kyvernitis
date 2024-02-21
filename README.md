# Kyvernitis
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/6655b26b23504686975095e1752b81cf)](https://app.codacy.com/gh/waseemR02/kyvernitis/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)

Zephyr Application  for R24 to program WeAct Blackpill F401cc as daughter board (for pixhawk).

Repo contains source files for each subsystem.

## Directory Structure
```
kyvernitis-ws/
│
├── kyvernitis/       │ <== Current Repo
│    ├── bio-arm
│    │    ├── boards
│    │    │   └── app.overlay
│    │    ├── CMakeLists.txt
│    │    ├── Kconfig
│    │    ├── prj.conf
│    │    └── src
│    │        └── main.c
│    ├── sub-system
│    │    ├── ...
│    │
│    ├── dts
│    │   └── bindings
│    │       └── pwm-motors.yaml
│    │       └── ...
│    ├── kyvernitis.c
│    ├── kyvernitis.h
│    ├── scripts
│    │    ├── ...
│    │
│    ├── tests
│    │    ├── ... 
│    │
│    ├── LICENSE
│    ├── README.md
│    ├── west.yml
│    └── zephyr
│        └── module.yml
│        └── Kconfig
│        └── CMakeLists.txt          
│                                   
├── modules/
│   └── lib/
│       └── canard/
│   └── hal/
│       └── ...     
│
└── zephyr/              
    └── west.yml         
                         
```

## Installation
This application contains a manifest file so it will follow a T2 topology workspace (refer to Zephyr docs)

**Assuming you have all the dependencies for zephyr development as well as the toolchain from the official release**


```
mkdir kyvernitis-ws
cd kyvernitis-ws
```


Create and activate virtual environment (can skip this step if you already have one) to install west
```
python3 -m venv .venv
source .venv/bin/activate
pip install west
```


Init the workspace with the manifest repo and pull modules from upstream
```
west init -m https://github.com/waseemR02/kyvernitis 
west config --global update.narrow true
west update
pip install -r zephyr/scripts/requirements-base.txt
source zephyr/zephyr-env.sh 
```


If everything is installed successfully then it should build any working sub-system without any errors
```
west build -p always -o=-j4 -b blackpill_f401cc kyvernitis/bio-arm
```

## Helpers
To view init sequence of the application you built, run
```
west build -d build -t initlevels
```
