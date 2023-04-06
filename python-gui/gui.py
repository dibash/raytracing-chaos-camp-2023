import tkinter as tk
import tkinter.messagebox
import customtkinter
import math
from PIL import Image, ImageTk

import ctypes

from CTkColorPicker import AskColor

customtkinter.set_appearance_mode("System")  # Modes: "System" (standard), "Dark", "Light"
customtkinter.set_default_color_theme("dark-blue")  # Themes: "blue" (standard), "green", "dark-blue"

SIDEBAR_WIDTH = 160
VIEWPORT_WIDTH = 1920
VIEWPORT_HEIGHT = 1080

dll = ctypes.WinDLL(r'D:\dev\raytracing_2023\library\build\Release\renderer_lib.dll')
dll.render.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_float]

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

        self.raw_pixels = bytearray(3 * VIEWPORT_WIDTH * VIEWPORT_HEIGHT)
        self.c_buffer = (ctypes.c_ubyte * len(self.raw_pixels)).from_buffer(self.raw_pixels)
        dll.render(self.c_buffer, 0)

        self.pixels = Image.new('RGB', (VIEWPORT_WIDTH, VIEWPORT_HEIGHT), (0, 0, 163))
        self.pixels.frombytes(bytes(self.raw_pixels))
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
        self.sidebar_button_1 = customtkinter.CTkButton(self.sidebar_frame, command=self.render_button_event)
        self.sidebar_button_1.grid(row=2, column=0, padx=20, pady=10)

        self.slider_1.set(0)
        self.slider_1.configure(command=lambda _: self.render_button_event())

    def render_button_event(self):
        dll.render(self.c_buffer, self.slider_1.get())
        self.pixels.frombytes(bytes(self.raw_pixels))
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
    app = App()
    app.mainloop()
