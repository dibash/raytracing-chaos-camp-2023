import bpy
import gpu
from gpu_extras.presets import draw_texture_2d

import array
import ctypes
import distutils
import os
import time


RENDERER_LIB_NAME = 'renderer_lib' + distutils.ccompiler.new_compiler().shared_lib_extension
RENDERER_LIB_FULL_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), os.path.pardir, 'install', RENDERER_LIB_NAME))


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
        self.dll.render.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_float]
        self.dll.render2.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int]

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


        # Export mesh data
        mesh_export_start = time.time()
        obj = scene.objects['Suzanne']
        mesh = obj.to_mesh()

        # Vertices
        vertices = [f for v in mesh.vertices for f in obj.matrix_world @ v.co]
        c_vert_arr = (ctypes.c_float * len(vertices))(*vertices)
        
        #console_print(vertices)
        
        # Triangles
        triangles = [i for tri in map(lambda t: [*t.vertices], mesh.loop_triangles) for i in tri]
        c_tri_arr = (ctypes.c_int * len(triangles))(*triangles)
        c_tri_cnt = ctypes.c_int(len(mesh.loop_triangles))
        
        #console_print(triangles)
        
        # Clear bpy mesh data
        obj.to_mesh_clear()
        
        console_print('export mesh time:', time.time() - mesh_export_start)
        
        # Create pixel buffer
        raw_pixels = bytearray(3 * 1920 * 1080)
        c_buffer = (ctypes.c_ubyte * len(raw_pixels)).from_buffer(raw_pixels)
        
        # Render the image
        render_image_start = time.time()
        self.dll.render2(c_buffer, c_vert_arr, c_tri_arr, c_tri_cnt)
        #self.dll.render(c_buffer, 0)
        console_print('render time:', time.time() - render_image_start)
        
        
        # Convert pixel data
        convert_pixels_start = time.time()
        pixel_count = 1920*1080
        rect = [[0,0,0,0]] * pixel_count
        rectIdx = 0
        rawIter = iter(raw_pixels)
        for idx,(r,g,b) in enumerate(zip(rawIter, rawIter, rawIter)):
            color = [r/256.0, g/256.0, b/256.0, 1.0]
            # flip the image
            pixPos = (1080 - 1 - idx//1920) * 1920 + idx%1920
            rect[pixPos] = color    
        console_print('convert pixels time:', time.time() - convert_pixels_start)

        if False:
            convert_pixels_start2 = time.time()
            rect = [[raw_pixels[i]/256, raw_pixels[i+1]/256, raw_pixels[i+2]/256, 1.0] for i in range(0, len(raw_pixels), 3)] 
            console_print('convert pixels time 2:', time.time() - convert_pixels_start2)

        # Write pixel data
        write_pixels_start = time.time()
        result = self.begin_result(0, 0, 1920, 1080)
        layer = result.layers[0].passes["Combined"]
        layer.rect = rect
        self.end_result(result)
        console_print('write pixels time:', time.time() - write_pixels_start)

    # For viewport renders, this method gets called once at the start and
    # whenever the scene or 3D viewport changes. This method is where data
    # should be read from Blender in the same thread. Typically a render
    # thread will be started to do the work while keeping Blender responsive.
    def view_update(self, context, depsgraph):
        region = context.region
        view3d = context.space_data
        scene = depsgraph.scene

        # Get viewport dimensions
        dimensions = region.width, region.height

        if not self.scene_data:
            # First time initialization
            self.scene_data = []
            first_time = True

            # Loop over all datablocks used in the scene.
            for datablock in depsgraph.ids:
                pass
        else:
            first_time = False

            # Test which datablocks changed
            for update in depsgraph.updates:
                print("Datablock updated: ", update.id.name)

            # Test if any material was added, removed or changed.
            if depsgraph.id_type_updated('MATERIAL'):
                print("Materials updated")

        # Loop over all object instances in the scene.
        if first_time or depsgraph.id_type_updated('OBJECT'):
            for instance in depsgraph.object_instances:
                pass

    # For viewport renders, this method is called whenever Blender redraws
    # the 3D viewport. The renderer is expected to quickly draw the render
    # with OpenGL, and not perform other expensive work.
    # Blender will draw overlays for selection and editing on top of the
    # rendered image automatically.
    def view_draw(self, context, depsgraph):
        region = context.region
        scene = depsgraph.scene

        # Get viewport dimensions
        dimensions = region.width, region.height

        # Bind shader that converts from scene linear to display space,
        gpu.state.blend_set('ALPHA_PREMULT')
        self.bind_display_space_shader(scene)

        if not self.draw_data or self.draw_data.dimensions != dimensions:
            self.draw_data = CustomDrawData(dimensions)

        self.draw_data.draw()

        self.unbind_display_space_shader()
        gpu.state.blend_set('NONE')


class CustomDrawData:
    def __init__(self, dimensions):
        # Generate dummy float image buffer
        self.dimensions = dimensions
        width, height = dimensions

        pixels = width * height * array.array('f', [0.1, 0.2, 0.1, 1.0])
        pixels = gpu.types.Buffer('FLOAT', width * height * 4, pixels)

        # Generate texture
        self.texture = gpu.types.GPUTexture((width, height), format='RGBA16F', data=pixels)

        # Note: This is just a didactic example.
        # In this case it would be more convenient to fill the texture with:
        # self.texture.clear('FLOAT', value=[0.1, 0.2, 0.1, 1.0])

    def __del__(self):
        del self.texture

    def draw(self):
        draw_texture_2d(self.texture, (0, 0), self.texture.width, self.texture.height)


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
