bl_info = {
    "name": "Export Project Orbit Scene",
    "blender": (2, 80, 0),
    "category": "Import-Export",
}

import bpy
import json
from bpy_extras.io_utils import ExportHelper


def get_asset_id(library):
    lib_path = library.filepath
    index = lib_path.find("database") + len("database")
    asset_id = lib_path[index:]
    index = asset_id.rfind("asset.blend")
    asset_id = asset_id[:index]
    asset_id = asset_id[1:-1]
    return int(asset_id)


class ExportGLBWithSkip(bpy.types.Operator, ExportHelper):
    bl_idname = "export_scene.project_orbit_glb"
    bl_label = "Export Scene"
    bl_options = {'REGISTER', 'UNDO'}
    filename_ext = ".glb"
    filter_glob: bpy.props.StringProperty(default="*.glb", options={'HIDDEN'})

    def execute(self, context):
        filepath = self.filepath
        # Get a list of objects to export (skip objects with 'project_orbit_id' property)
        objects_to_export = [obj for obj in bpy.context.scene.objects if not obj.override_library]
        
        # Select only the objects to be exported
        initially_selected_objects = [obj.name for obj in bpy.context.selected_objects]
        bpy.ops.object.select_all(action='DESELECT')
        for obj in objects_to_export:
            obj.select_set(True)
        
        # Create a dictionary of assets to be included in the scene
        library_objects_data = []
        library_objects = [obj for obj in bpy.context.scene.objects if obj.override_library]
        for obj in library_objects:
            reference_obj = obj.override_library.reference
            if reference_obj is None:
                print("Faulty object: " + obj.name)
            reference_node_name = reference_obj.name
            library = reference_obj.library
            matrix_world = obj.matrix_world
            location_gltf = matrix_world.translation
            rotation_gltf = matrix_world.to_quaternion()
            scale_gltf = matrix_world.to_scale()
            location_gltf = [location_gltf.x, location_gltf.z, -location_gltf.y]
            rotation_gltf = [rotation_gltf.w, rotation_gltf.x, rotation_gltf.z, -rotation_gltf.y]
            scale_gltf = [scale_gltf.x, scale_gltf.z, scale_gltf.y]
            obj_data = {
                "name": obj.name,
                "type": obj.type,
                "location": location_gltf,
                "rotation": rotation_gltf,
                "scale": scale_gltf,
                "mesh": { "node_name": reference_obj.name, "asset_id": get_asset_id(library) },
            }
            # save the parent if its not a root node
            #if obj.parent:
            #    obj_data["parent"] = obj.parent.name
            # save the mesh data (e.g. materials) if its overriden
            if obj.data.override_library is not None:
                materials = []
                for i in range(len(reference_obj.material_slots)):
                    original_material = reference_obj.material_slots[i].material
                    replacement_material = obj.material_slots[i].material
                    slot = { "original_name": original_material.name, "replacement_name": replacement_material.name, "asset_id": get_asset_id(replacement_material.library) }
                    materials.append(slot)
                obj_data["library_materials_override"] = materials
            # append
            library_objects_data.append(obj_data)
        bpy.context.scene['project_orbit_data'] = { "library_objects": library_objects_data }

        # Export the GLB file
        bpy.ops.export_scene.gltf(
            filepath=filepath,
            export_format='GLB',
            use_selection=True,
            export_extras=True)
        
        # Reselect the initially selected objects
        bpy.ops.object.select_all(action='DESELECT')
        for obj_name in initially_selected_objects:
            bpy.context.scene.objects[obj_name].select_set(True)
        
        return {'FINISHED'}

def menu_func_export(self, context):
    self.layout.operator(ExportGLBWithSkip.bl_idname, text="Project Orbit Scene (.glb)")

def register():
    bpy.utils.register_class(ExportGLBWithSkip)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)
    bpy.types.Scene.custom_data = bpy.props.PointerProperty(type=bpy.types.Text)

def unregister():
    bpy.utils.unregister_class(ExportGLBWithSkip)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)
    del bpy.types.Scene.custom_data

if __name__ == "__main__":
    register()
