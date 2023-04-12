import bpy

import os
import ctypes
import distutils
import timeit
import numpy as np

RENDERER_LIB_FNAME = 'renderer_lib' + distutils.ccompiler.new_compiler().shared_lib_extension
RENDERER_LIB_PATH = os.path.abspath(os.getenv('CHAOS_RAYTRACING_LIB_PATH', default=os.path.join(os.path.dirname(__file__), os.path.pardir, 'install', 'lib')))
RENDERER_LIB_FULL_PATH = os.path.join(RENDERER_LIB_PATH, RENDERER_LIB_FNAME)

VIEWPORT_WIDTH = 1920
VIEWPORT_HEIGHT = 1080
VIEWPORT_CHANNELS = 4

LOG_TIME = True


def console_print(*args, **kwargs):
    for window in bpy.context.window_manager.windows:
        screen = window.screen
        for a in screen.areas:
            if a.type == 'CONSOLE':
                c = {}
                c['area'] = a
                c['space_data'] = a.spaces.active
                c['region'] = a.regions[-1]
                c['window'] = window
                c['screen'] = screen
                s = " ".join([str(arg) for arg in args])
                for line in s.split("\n"):
                    bpy.ops.console.scrollback_append(c, text=line)


class CodeTimer:
    def __init__(self, name=None):
        self.name = "'"  + name + "'" if name else ''

    def __enter__(self):
        if LOG_TIME:
            self.start = timeit.default_timer()

    def __exit__(self, exc_type, exc_value, traceback):
        if LOG_TIME:
            self.took = (timeit.default_timer() - self.start) * 1000.0
            console_print(f'Code block {self.name} took: {self.took:.0f} ms')


class CustomRenderEngine(bpy.types.RenderEngine):
    # These three members are used by blender to set up the
    # RenderEngine; define its internal name, visible name and capabilities.
    bl_idname = "CUSTOM"
    bl_label = "MyCustomEngine"
    bl_use_preview = False
    bl_use_eevee_viewport = True

    # Init is called whenever a new render engine instance is created. Multiple
    # instances may exist at the same time, for example for a viewport and final
    # render.
    def __init__(self):
        self.scene_data = None
        self.draw_data = None
        self.dll = ctypes.WinDLL(RENDERER_LIB_FULL_PATH)
        self.dll.render.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_float]
        self.dll.render2.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int]

    # When the render engine instance is destroy, this is called. Clean up any
    # render engine data here, for example stopping running render threads.
    def __del__(self):
        #ctypes.windll.kernel32.FreeLibrary(self.dll._handle)
        pass

    # This is the method called by Blender for final renders (F12)
    def render(self, depsgraph):
        scene = depsgraph.scene
        #scale = scene.render.resolution_percentage / 100.0
        #self.size_x = int(scene.render.resolution_x * scale)
        #self.size_y = int(scene.render.resolution_y * scale)

        with CodeTimer('Export'):
            obj = scene.objects['Suzanne']
            mesh = obj.to_mesh()

            # Vertices
            vertices = [f for v in mesh.vertices for f in obj.matrix_world @ v.co]
            c_vert_arr = (ctypes.c_float * len(vertices))(*vertices)

            # Triangles
            triangles = [i for tri in map(lambda t: [*t.vertices], mesh.loop_triangles) for i in tri]
            c_tri_arr = (ctypes.c_int * len(triangles))(*triangles)
            c_tri_cnt = ctypes.c_int(len(mesh.loop_triangles))

            # Clear bpy mesh data
            obj.to_mesh_clear()

        with CodeTimer('Render'):
            c_buffer = (ctypes.c_float * (VIEWPORT_WIDTH * VIEWPORT_HEIGHT * VIEWPORT_CHANNELS))()

            # Render the image
            self.dll.render2(c_buffer, c_vert_arr, c_tri_arr, c_tri_cnt)
            #self.dll.render(c_buffer, 0)

        with CodeTimer('Write image'):
            # Flip pixel buffer
            np_array = np.frombuffer(c_buffer, dtype=np.float32)
            np_array = np_array.reshape((VIEWPORT_HEIGHT, VIEWPORT_WIDTH, VIEWPORT_CHANNELS))
            np_array = np.flipud(np_array)
            np_array = np_array.reshape((VIEWPORT_HEIGHT * VIEWPORT_WIDTH, VIEWPORT_CHANNELS))

            # Write pixel data
            result = self.begin_result(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT)
            layer = result.layers[0].passes["Combined"]
            layer.rect.foreach_set(np_array)
            self.end_result(result)


# RenderEngines also need to tell UI Panels that they are compatible with.
# We recommend to enable all panels marked as BLENDER_RENDER, and then
# exclude any panels that are replaced by custom panels registered by the
# render engine, or that are not supported.
def get_panels():
    exclude_panels = {
        'VIEWLAYER_PT_filter',
        'VIEWLAYER_PT_layer_passes',
    }

    panels = []
    for panel in bpy.types.Panel.__subclasses__():
        if hasattr(panel, 'COMPAT_ENGINES') and 'BLENDER_RENDER' in panel.COMPAT_ENGINES:
            if panel.__name__ not in exclude_panels:
                panels.append(panel)

    return panels


def register():
    # Register the RenderEngine
    bpy.utils.register_class(CustomRenderEngine)

    for panel in get_panels():
        panel.COMPAT_ENGINES.add('CUSTOM')


def unregister():
    bpy.utils.unregister_class(CustomRenderEngine)

    for panel in get_panels():
        if 'CUSTOM' in panel.COMPAT_ENGINES:
            panel.COMPAT_ENGINES.remove('CUSTOM')


if __name__ == "__main__":
    register()
