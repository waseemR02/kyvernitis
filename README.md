# Kyvernitis
Zephyr Application  for R24 to program WeAct Blackpill F401cc as daughter board (for pixhawk)

## Usage
This application contains a manifest file so it will follow a T2 topology workspace (refer to Zephyr docs)

**Assumimg you have all the dependencies for zephyr development as well as the toolchain from the official release**


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
west init -m git@github.com:waseemR02/kyvernitis.git
west update --narrow
pip install -r zephyr/scripts/requirements-base.txt
source zephyr/zephyr-env.sh 
```


If everything is installed successfully then it should build the working src/main.c without any errors
```
west build -p always -o=-j4 -b blackpill_f401cc kyvernitis/bio-arm
```
