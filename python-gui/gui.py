import tkinter as tk
import customtkinter
import numpy as np
from PIL import Image, ImageTk

import ctypes
import distutils
import os
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

        with CodeTimer('Render'):
            self.c_buffer = (ctypes.c_float * (VIEWPORT_WIDTH * VIEWPORT_HEIGHT * VIEWPORT_CHANNELS))()
            dll.render(self.c_buffer, 0)

        with CodeTimer('Pixel conversion'):
            np_array = np.frombuffer(self.c_buffer, dtype=np.float32)
            np_array = np_array.reshape((VIEWPORT_HEIGHT, VIEWPORT_WIDTH, VIEWPORT_CHANNELS))
            self.pixels = Image.fromarray((np_array * 255.999).astype(np.uint8))

        with CodeTimer('Image creation'):
            self.img = ImageTk.PhotoImage(self.pixels)
            self.canvas.create_image((VIEWPORT_WIDTH/2, VIEWPORT_HEIGHT/2), image=self.img, state="normal")

        # create sidebar frame with widgets
        self.sidebar_frame = customtkinter.CTkFrame(self, width=SIDEBAR_WIDTH, corner_radius=0)
        self.sidebar_frame.grid(row=0, column=1, sticky="nsew")
        self.sidebar_frame.grid_rowconfigure(4, weight=1)
        self.logo_label = customtkinter.CTkLabel(self.sidebar_frame, text="Ray Tracing 2023", font=customtkinter.CTkFont(size=20, weight="bold"))
        self.logo_label.grid(row=0, column=0, padx=20, pady=(20, 10))

        self.slider_1 = customtkinter.CTkSlider(self.sidebar_frame, from_=10, to=-1, number_of_steps=100)
        self.slider_1.grid(row=1, column=0, padx=(20, 10), pady=(10, 10), sticky="ew")
        self.sidebar_button_1 = customtkinter.CTkButton(self.sidebar_frame, text='Render', command=self.render_button_event)
        self.sidebar_button_1.grid(row=2, column=0, padx=20, pady=10)

        self.slider_1.set(0)
        self.slider_1.configure(command=lambda _: self.render_button_event())

    def render_button_event(self):
        #with CodeTimer('Render'):
            dll.render(self.c_buffer, self.slider_1.get())

        #with CodeTimer('Pixel conversion'):
            np_array = np.frombuffer(self.c_buffer, dtype=np.float32)
            np_array = np_array.reshape((VIEWPORT_HEIGHT, VIEWPORT_WIDTH, VIEWPORT_CHANNELS))
            self.pixels = Image.fromarray((np_array * 255).astype(np.uint8))

        #with CodeTimer('Image copy'):
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
