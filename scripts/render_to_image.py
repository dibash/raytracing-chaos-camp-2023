import numpy as np
from PIL import Image

import ctypes
import distutils.ccompiler
import os
import sys

# Check for CHAOS_RAYTRACING_LIB_PATH env. variable, and use that if availbale
# Otherwise, try to load from the default install location
RENDERER_LIB_FNAME = 'renderer_lib' + distutils.ccompiler.new_compiler().shared_lib_extension
RENDERER_LIB_PATH = os.path.abspath(os.getenv('CHAOS_RAYTRACING_LIB_PATH', default=os.path.join(os.path.dirname(__file__), os.path.pardir, 'install', 'lib')))
RENDERER_LIB_FULL_PATH = os.path.join(RENDERER_LIB_PATH, RENDERER_LIB_FNAME)

dll = ctypes.CDLL(RENDERER_LIB_FULL_PATH)
dll.renderFile2.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_char_p, ctypes.c_int, ctypes.c_int]
dll.getSizeFromFile.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]


def render_image(fileName, width, height):

    c_fileName = ctypes.c_char_p(bytes(fileName, sys.getfilesystemencoding()))
    c_width = ctypes.c_int()
    c_height = ctypes.c_int()

    if width * height == 0:
        dll.getSizeFromFile(c_fileName, ctypes.byref(c_width), ctypes.byref(c_height))
        width, height = c_width.value, c_height.value

    c_buffer = (ctypes.c_float * (width * height * 4))()

    dll.renderFile2(c_buffer, c_fileName, width, height)

    np_array = np.frombuffer(c_buffer, dtype=np.float32)
    np_array = np_array.reshape((height, width, 4))
    np_array = np.clip(np_array, 0, 1)
    pixels = Image.fromarray((np_array * 255).astype(np.uint8))

    output_folder = os.path.join(os.path.dirname(fileName), 'output')
    os.makedirs(output_folder, exist_ok=True)
    output_file = os.path.join(output_folder, os.path.splitext(os.path.basename(fileName))[0] + ".png")
    pixels.save(output_file)


def render_folder(folder_path, width, height):
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            if file.endswith(".crtscene"):
                fileName = os.path.join(root, file)
                render_image(fileName, width, height)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python render_image.py <fileName/folder> [width] [height]")
    else:
        path = sys.argv[1]
        width = int(sys.argv[2]) if len(sys.argv) > 2 else 0
        height = int(sys.argv[3]) if len(sys.argv) > 3 else width
        if os.path.isfile(path):
            render_image(path, width, height)
        elif os.path.isdir(path):
            render_folder(path, width, height)
        else:
            print(f"Invalid input: {path} is not a valid file or folder path.")

