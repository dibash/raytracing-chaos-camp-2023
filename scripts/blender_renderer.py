import bpy

import os
import ctypes
import distutils
import timeit
import json
import numpy as np
import tempfile
from mathutils import Matrix


RENDERER_LIB_FNAME = 'renderer_lib' + distutils.ccompiler.new_compiler().shared_lib_extension
RENDERER_LIB_PATH = os.path.abspath(os.getenv('CHAOS_RAYTRACING_LIB_PATH', default=os.path.join(os.path.dirname(__file__), os.path.pardir, 'install', 'lib')))
RENDERER_LIB_FULL_PATH = os.path.join(RENDERER_LIB_PATH, RENDERER_LIB_FNAME)

LOG_TIME = True


def export_scene_to_json(scene, filepath):
    scale = scene.render.resolution_percentage / 100.0
    data = {
        'settings': {
            'background_color': list(scene.world.color),
            'image_settings': {
                'width': int(scene.render.resolution_x * scale),
                'height': int(scene.render.resolution_y * scale)
            }
        },
        'camera': {
            'matrix': [num for v in Matrix.transposed(scene.camera.matrix_world.to_3x3()) for num in v],
            'position': list(scene.camera.location)
        },
        'lights': [],
        'materials': [],
        'objects': []
    }

    for lamp in bpy.data.lights:
        if lamp.type == 'POINT' and lamp.users > 0:
            light_data = {
                'intensity': lamp.energy,
                'position': list(scene.objects[lamp.name_full].matrix_world.translation)
            }
            data['lights'].append(light_data)

    for obj in bpy.data.objects:
        if obj.type == 'MESH' and obj.users > 0 and not obj.hide_get():
            material = obj.active_material
            material_node = material.node_tree.get_output_node('ALL').inputs['Surface'].links[0].from_node
            material_color = list(material_node.inputs['Color'].default_value)
            # Use the first face to decide if it should be smooth.
            use_smooth = obj.data.loop_triangles[0].use_smooth
            material_data = {
                'type': 'diffuse', # default
                'albedo': material_color,
                'smooth_shading': use_smooth
            }
            if material_node.type == 'BSDF_GLOSSY':
                material_data['type'] = 'reflective'
            if material_node.type == 'BSDF_GLASS':
                material_data['type'] = 'refractive'
                material_data['ior'] = material_node.inputs['IOR'].default_value
            data['materials'].append(material_data)

            obj_data = {
                'material_index': len(data['materials']) - 1,
                'vertices': [],
                'triangles': []
            }

            obj_data['vertices'] = [n for v in obj.data.vertices for n in obj.matrix_world @ v.co]

            for face in obj.data.loop_triangles:
                for vert_index in face.vertices:
                    obj_data['triangles'].append(vert_index)

            data['objects'].append(obj_data)

    with open(filepath, 'w') as file:
        json.dump(data, file, indent=4)


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
    bl_idname = "CRT2023"
    bl_label = "CRT2023"
    bl_use_preview = False
    bl_use_eevee_viewport = True

    # Init is called whenever a new render engine instance is created. Multiple
    # instances may exist at the same time, for example for a viewport and final
    # render.
    def __init__(self):
        self.scene_data = None
        self.draw_data = None
        self.dll = ctypes.CDLL(RENDERER_LIB_FULL_PATH)
        self.dll.renderFile.argtypes = [ctypes.POINTER(ctypes.c_float), ctypes.c_char_p]

    # When the render engine instance is destroy, this is called. Clean up any
    # render engine data here, for example stopping running render threads.
    def __del__(self):
        del self.c_buffer
        pass

    # This is the method called by Blender for final renders (F12)
    def render(self, depsgraph):
        scene = depsgraph.scene
        scale = scene.render.resolution_percentage / 100.0
        self.size_x = int(scene.render.resolution_x * scale)
        self.size_y = int(scene.render.resolution_y * scale)

        console_print(f'resolution: {self.size_x}x{self.size_y}')

        with CodeTimer('Export'):
            with tempfile.NamedTemporaryFile(suffix='.json', delete=False) as temp_file:
                self.scene_path = temp_file.name
                export_scene_to_json(scene, self.scene_path)

        with CodeTimer('Render'):
            self.c_buffer = (ctypes.c_float * (self.size_x * self.size_y * 4))()
            self.dll.renderFile(self.c_buffer, ctypes.c_char_p(self.scene_path.encode('utf-8')))

        with CodeTimer('Write image'):
            # Flip pixel buffer
            np_array = np.frombuffer(self.c_buffer, dtype=np.float32)
            np_array = np_array.reshape((self.size_y, self.size_x, 4))
            np_array = np.flipud(np_array)
            np_array = np_array.reshape((self.size_y * self.size_x, 4))

            # Write pixel data
            result = self.begin_result(0, 0, self.size_x, self.size_y)
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
