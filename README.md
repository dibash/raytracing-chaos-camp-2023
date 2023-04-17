# Ray Tracing course 2023 (Chaos Camp)

A simple raytracer project for the [2023 Ray Tracing course of Chaos Camp](https://www.chaos.com/chaos-camp/ray-tracing-course-2023)

## To build:
Just run CMake

```cmake
cmake .. -DPYTHON_EXECUTABLE=C:\Python310\python.exe
```

### CMake options:

|      Option       | Default value | Description |
|        ---        |      ---      |     ---     |
| SETUP_PYTHON_ENV  |       ON      | Whether to setup the Python virtual environment. |
| PYTHON_EXECUTABLE |   python.exe  | Path to the Python executable. For linux/macos this _must_ be set explicitly. |

## Python GUI app

If `SETUP_PYTHON_ENV` is `ON`, CMake will create a Python virtual environment `.venv` inside the build directory.
The required modules will be installed in that virtual environment.

> Note: Python 3 is required with tkinter installed!

Before run the app, you should set `CHAOS_RAYTRACING_LIB_PATH` environment variable, or run the `install` CMake target.  
Then, activate the environment for the shell you are using:
```
.venv/Scripts/activate.ps1
```
After that, just run `gui.py` with `python`:
```
python gui.py
```

> If using Visual Studio, you can set renderer_lib as startup project, and start the debugger.
> The Python GUI app will be started with the proper environment set, and the debugger should be attached to the process.
