import tkinter as tk
from tkinter import filedialog
import customtkinter
import numpy as np
from PIL import Image, ImageTk

import ctypes
import distutils.ccompiler
import os
import sys
import timeit

from CTkColorPicker import AskColor

# Check for CHAOS_RAYTRACING_LIB_PATH env. variable, and use that if availbale
# Otherwise, try to load from the default install location
RENDERER_LIB_FNAME = 'renderer_lib' + distutils.ccompiler.new_compiler().shared_lib_extension
RENDERER_LIB_PATH = os.path.abspath(os.getenv('CHAOS_RAYTRACING_LIB_PATH', default=os.path.join(os.path.dirname(__file__), os.path.pardir, 'install', 'lib')))
RENDERER_LIB_FULL_PATH = os.path.join(RENDERER_LIB_PATH, RENDERER_LIB_FNAME)

LOG_TIME = True

SIDEBAR_WIDTH = 160
VIEWPORT_WIDTH = 1920
VIEWPORT_HEIGHT = 1080
VIEWPORT_CHANNELS = 4

dll = ctypes.WinDLL(RENDERER_LIB_FULL_PATH)
dll.render.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_float]
dll.renderCamera.argtypes = [ctypes.POINTER(ctypes.c_float)] + [ctypes.c_float] * 7
dll.renderFile.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_char_p]


class CodeTimer:
    def __init__(self, name=None):
        self.name = "'"  + name + "'" if name else ''

    def __enter__(self):
        if LOG_TIME:
            self.start = timeit.default_timer()

    def __exit__(self, exc_type, exc_value, traceback):
        if LOG_TIME:
            self.took = (timeit.default_timer() - self.start) * 1000.0
            print(f'Code block {self.name} took: {self.took:.0f} ms')


class App(customtkinter.CTk):
    def __init__(self):
        super().__init__()

        # configure window
        self.title("Test application window")
        self.geometry(f"{VIEWPORT_WIDTH + SIDEBAR_WIDTH}x{VIEWPORT_HEIGHT}")
        self.resizable(height=False, width=False)

        # configure grid layout (4x4)
        self.grid_columnconfigure(0, weight=1)
        self.grid_columnconfigure(1, weight=0)
        self.grid_rowconfigure(0, weight=1)

        # create image viewport
        self.viewport_frame = customtkinter.CTkFrame(self, corner_radius=0)
        self.viewport_frame.grid(row=0, column=0, sticky='nsew')

        self.canvas = customtkinter.CTkCanvas(self.viewport_frame, width=VIEWPORT_WIDTH, height=VIEWPORT_HEIGHT, highlightthickness=0, bg="#0000A3")
        self.canvas.pack(padx=0, pady=0)

        # create sidebar frame with widgets
        self.sidebar_frame = customtkinter.CTkFrame(self, width=SIDEBAR_WIDTH, corner_radius=0)
        self.sidebar_frame.grid(row=0, column=1, sticky="nsew")
        self.sidebar_frame.grid_rowconfigure(10, weight=1)
        self.logo_label = customtkinter.CTkLabel(self.sidebar_frame, text="Ray Tracing 2023", font=customtkinter.CTkFont(size=20, weight="bold"))
        self.logo_label.grid(row=0, column=0, padx=20, pady=(20, 10))

        self.sliders = {}
        self.addSlider(1, 'x',     0,  -10,   10, 100)
        self.addSlider(2, 'y',     0,  -10,   10, 100)
        self.addSlider(3, 'z',     0,  -10,   10, 100)
        self.addSlider(4, 'fov',  90,    0,  180, 180)
        self.addSlider(5, 'pan',   0, -180,  180, 360)
        self.addSlider(6, 'tilt',  0,  -90,   90, 180)
        self.addSlider(7, 'roll',  0,  -90,   90, 180)

        self.sidebar_button_1 = customtkinter.CTkButton(self.sidebar_frame, text='Render', command=self.render_button_event)
        self.sidebar_button_1.grid(row=8, column=0, padx=20, pady=10)

        self.sidebar_button_2 = customtkinter.CTkButton(self.sidebar_frame, text='Load scene', command=self.load_scene_event)
        self.sidebar_button_2.grid(row=9, column=0, padx=20, pady=10)

        with CodeTimer('renderCamera'):
            self.c_buffer = (ctypes.c_float * (VIEWPORT_WIDTH * VIEWPORT_HEIGHT * VIEWPORT_CHANNELS))()
            x = self.sliders['x'].slider.get()
            y = self.sliders['y'].slider.get()
            z = self.sliders['z'].slider.get()
            fov = self.sliders['fov'].slider.get()
            pan = self.sliders['pan'].slider.get()
            tilt = self.sliders['tilt'].slider.get()
            roll = self.sliders['roll'].slider.get()

            dll.renderCamera(self.c_buffer, x, y, z, fov, pan, tilt, roll)

            np_array = np.frombuffer(self.c_buffer, dtype=np.float32)
            np_array = np_array.reshape((VIEWPORT_HEIGHT, VIEWPORT_WIDTH, VIEWPORT_CHANNELS))
            self.pixels = Image.fromarray((np_array * 255.999).astype(np.uint8))

            self.img = ImageTk.PhotoImage(self.pixels)
            self.canvas.create_image((VIEWPORT_WIDTH/2, VIEWPORT_HEIGHT/2), image=self.img, state="normal")

    def addSlider(self, row, name, v, vmin, vmax, vsteps):
        slider_frame = customtkinter.CTkFrame(self.sidebar_frame, corner_radius=0)
        slider_frame.grid(row=row)
        label = customtkinter.CTkLabel(slider_frame, text=f'{name}: {v}')
        label.pack()
        slider = customtkinter.CTkSlider(slider_frame, from_=vmin, to=vmax, number_of_steps=vsteps)
        slider.pack(padx=(20, 10), pady=(10, 10))
        def command(val):
            label.configure(require_redraw=True, text=f'{name}: {val:.0f}')
            self.render_button_event()
        slider.configure(command=command)
        slider.set(v)
        class Slider(object):
            pass
        slider_obj = Slider()
        slider_obj.slider = slider
        slider_obj.label = label
        self.sliders[name] = slider_obj


    def render_button_event(self):
        x = self.sliders['x'].slider.get()
        y = self.sliders['y'].slider.get()
        z = self.sliders['z'].slider.get()
        fov = self.sliders['fov'].slider.get()
        pan = self.sliders['pan'].slider.get()
        tilt = self.sliders['tilt'].slider.get()
        roll = self.sliders['roll'].slider.get()

        with CodeTimer('renderCamera'):
            dll.renderCamera(self.c_buffer, x, y, z, fov, pan, tilt, roll)

        np_array = np.frombuffer(self.c_buffer, dtype=np.float32)
        np_array = np_array.reshape((VIEWPORT_HEIGHT, VIEWPORT_WIDTH, VIEWPORT_CHANNELS))
        self.pixels = Image.fromarray((np_array * 255).astype(np.uint8))

        self.img.paste(self.pixels)

    def load_scene_event(self):
        filetypes = (
            ('CRTScene files', '*.crtscene'),
            ('All files', '*.*'),
        )
        fileName = filedialog.askopenfilename(
            title='Choose a scene to render',
            filetypes=filetypes
        )

        with CodeTimer(f'Render {fileName}'):
            dll.renderFile(self.c_buffer, ctypes.c_char_p(bytes(fileName, sys.getfilesystemencoding())))

        np_array = np.frombuffer(self.c_buffer, dtype=np.float32)
        np_array = np_array.reshape((VIEWPORT_HEIGHT, VIEWPORT_WIDTH, VIEWPORT_CHANNELS))
        self.pixels = Image.fromarray((np_array * 255).astype(np.uint8))
        self.img.paste(self.pixels)

    def set_color_button_event(self):
        pick_color = AskColor() # Open the Color Picker
        pick_color.get()
        color = pick_color.rgb_color # Get the color
        print(color)
        print(tuple(c for c in color))
        self.pixels.paste(tuple(c for c in color), [0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT])
        self.img.paste(self.pixels)


if __name__ == "__main__":
    customtkinter.set_appearance_mode("System")  # Modes: "System" (standard), "Dark", "Light"
    customtkinter.set_default_color_theme("dark-blue")  # Themes: "blue" (standard), "green", "dark-blue"
    app = App()
    app.mainloop()
