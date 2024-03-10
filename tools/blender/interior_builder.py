import bpy
import math
import json

import mathutils

assets_path = "../../assets/"
floor_file_path = assets_path + "environment/hallway_floor_00.glb"
wall_file_path = assets_path + "environment/hallway_wall_00.glb"
ceiling_file_path = assets_path + "environment/hallway_ceiling_00.glb"

cell_width = 1.0
cell_depth = 1.0
cell_height = 2.5

selected_blueprint_path = ""
root_object = None


def import_gltf_and_get_object(filepath):
    bpy.ops.import_scene.gltf(filepath=filepath)
    imported_objects = bpy.context.selected_objects
    imported_object = next((obj for obj in imported_objects if obj.data is not None), None)
    return imported_object


class FileBrowserOperator(bpy.types.Operator):
    bl_idname = "custom.file_browser"
    bl_label = "Load Blueprint"
    filepath: bpy.props.StringProperty(subtype="FILE_PATH")
    filter_glob: bpy.props.StringProperty(
        default="*.json",
        options={'HIDDEN'},
        maxlen=255,
    )

    def execute(self, context):
        global selected_blueprint_path
        selected_blueprint_path = self.filepath
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}


class InteriorProperties(bpy.types.PropertyGroup):
    cell_width: bpy.props.FloatProperty(name="Cell Width", default=1.0)
    cell_height: bpy.props.FloatProperty(name="Cell Height", default=1.0)
    cell_depth: bpy.props.FloatProperty(name="Cell Depth", default=1.0)

class ConstructShipOperator(bpy.types.Operator):
    bl_idname = "object.construct_ship"
    bl_label = "Load Partial Model"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context):
        global selected_blueprint_path
        return selected_blueprint_path != ""

    def execute(self, context):
        # Build the cell-based representation
        global selected_blueprint_path
        x_min = 0; y_min = 0; z_min = 0
        x_max = 0; y_max = 0; z_max = 0
        cell_data = {}
        with open(selected_blueprint_path, 'r') as file:
            json_data = json.load(file)
            volumes = json_data["volumes"]
            for v in volumes:
                x_start = min(v["x_start"], v["x_end"])
                x_end = max(v["x_start"], v["x_end"])
                y_start = min(v["y_start"], v["y_end"])
                y_end = max(v["y_start"], v["y_end"])
                z_start = min(v["z_start"], v["z_end"])
                z_end = max(v["z_start"], v["z_end"])
                x_min = min(x_min, x_start)
                y_min = min(y_min, y_start)
                z_min = min(z_min, z_start)
                x_max = max(x_max, x_end)
                y_max = max(y_max, y_end)
                z_max = max(z_max, z_end)
                for x in range(x_start, x_end):
                    for y in range(y_start, y_end):
                        for z in range(z_start, z_end):
                            cell_data[(x,y,z)] = True
        # Construct with meshes
        imported_floor_object = import_gltf_and_get_object(floor_file_path)
        imported_wall_object = import_gltf_and_get_object(wall_file_path)
        imported_ceiling_object = import_gltf_and_get_object(ceiling_file_path)
        global root_object
        root_object = bpy.data.objects.new("interior_root", None)
        bpy.context.collection.objects.link(root_object)
        root_object.location = (0.0, 0.0, 0.0)
        root_object["cell_width"] = cell_width
        root_object["cell_depth"] = cell_depth
        root_object["cell_height"] = cell_height
        bpy.context.view_layer.objects.active = root_object
        for z in range(z_min, z_max):
            level_object = bpy.data.objects.new("level " + str(z), None)
            bpy.context.collection.objects.link(level_object)
            level_object.location = (0.0, 0.0, z*cell_height)
            level_object["floor_number"] = z
            level_object.parent = root_object
            for x in range(x_min, x_max):
                for y in range(y_min, y_max):
                    if cell_data.get((x,y,z), False) == True:
                        # node
                        cell_object = bpy.data.objects.new("cell", None)
                        bpy.context.collection.objects.link(cell_object)
                        cell_object.location = (x*cell_width, y*cell_depth, 0.0)
                        cell_object["cell_x"] = x
                        cell_object["cell_y"] = y
                        cell_object.parent = level_object
                        if context.scene.add_meshes:
                            # floor
                            new_instance = imported_floor_object.copy()
                            bpy.context.collection.objects.link(new_instance)
                            new_instance.parent = cell_object
                            # walls
                            axis = (0.0, 0.0, 1.0)
                            if cell_data.get((x-1,y,z), False) == False: # wall right
                                new_instance = imported_wall_object.copy()
                                bpy.context.collection.objects.link(new_instance)
                                new_instance.parent = cell_object
                                new_instance.name = "wall_right"
                                new_instance.rotation_euler = (0.0, 0.0, 0.0)
                            if cell_data.get((x+1,y,z), False) == False: # wall left
                                new_instance = imported_wall_object.copy()
                                bpy.context.collection.objects.link(new_instance)
                                new_instance.parent = cell_object
                                new_instance.name = "wall_left"
                                rotation_quaternion = mathutils.Quaternion(axis, math.radians(180))
                                new_instance.rotation_quaternion = rotation_quaternion
                            if cell_data.get((x,y+1,z), False) == False: # wall up
                                new_instance = imported_wall_object.copy()
                                bpy.context.collection.objects.link(new_instance)
                                new_instance.parent = cell_object
                                new_instance.name = "wall_up"
                                rotation_quaternion = mathutils.Quaternion(axis, math.radians(-90))
                                new_instance.rotation_quaternion = rotation_quaternion
                            if cell_data.get((x,y-1,z), False) == False: # wall down
                                new_instance = imported_wall_object.copy()
                                bpy.context.collection.objects.link(new_instance)
                                new_instance.parent = cell_object
                                new_instance.name = "wall_down"
                                rotation_quaternion = mathutils.Quaternion(axis, math.radians(90))
                                new_instance.rotation_quaternion = rotation_quaternion

        # Delete the imported_objects
        bpy.data.objects.remove(imported_floor_object, do_unlink=True)
        bpy.data.objects.remove(imported_wall_object, do_unlink=True)
        bpy.data.objects.remove(imported_ceiling_object, do_unlink=True)
        return {'FINISHED'}
        
#
# Delete operator
#
class DeleteObjectOperator(bpy.types.Operator):
    bl_idname = "object.delete_object"
    bl_label = "Delete Object"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context):
        global root_object
        return root_object is not None

    def execute(self, context):
        global root_object
        self.recursive_delete(root_object)
        root_object = None
        return {'FINISHED'}
    
    def recursive_delete(self, obj):
        if obj is not None:
            for child in obj.children:
                self.recursive_delete(child)
            bpy.data.objects.remove(obj, do_unlink=True)


#
# Main panel
#
class InteriorBuilder(bpy.types.Panel):
    bl_label = "Interior Builder"
    bl_idname = "PT_InteriorBuilder"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "ProjectOrbit"
    
    def draw(self, context):
        layout = self.layout
        row = layout.row()
        layout.operator("custom.file_browser", text="Open Blueprint")
        row = layout.row()
        layout.prop(context.scene, "add_meshes", text="Add Meshes")
        row = layout.row()
        layout.operator(ConstructShipOperator.bl_idname, text="Construct")
        row = layout.row()
        layout.operator(DeleteObjectOperator.bl_idname, text="Delete")
        

def register():
    bpy.utils.register_class(FileBrowserOperator)
    bpy.utils.register_class(InteriorBuilder)
    bpy.utils.register_class(DeleteObjectOperator)
    bpy.utils.register_class(ConstructShipOperator)
    bpy.types.Scene.add_meshes = bpy.props.BoolProperty(
        name="Add Meshes",
        default=False,
        description="Enable or disable mesh adding to the construction"
    )
    
    
def unregister():
    bpy.utils.unregister_class(FileBrowserOperator)
    bpy.utils.unregister_class(InteriorBuilder)
    bpy.utils.unregister_class(DeleteObjectOperator)
    bpy.utils.unregister_class(ConstructShipOperator)
    del bpy.types.Scene.add_meshes
    
    
if __name__ == "__main__":
    register()