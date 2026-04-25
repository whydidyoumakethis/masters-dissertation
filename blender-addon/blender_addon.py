bl_info = {
    "name": "glTF Tag UI",
    "author": "whydidyoumakethis",
    "version": (1, 0, 0),
    "blender": (5, 0, 1),
    "location": "View3D > Sidebar > Game Tags",
    "description": "UI list and tagging tools for glTF extras workflow",
}

import bpy
from bpy.types import Panel, Operator, PropertyGroup, UIList
from bpy.props import (
    EnumProperty,
    CollectionProperty,
    IntProperty,
    PointerProperty,
)


# ---------------------------------------------------
# Preset item
# ---------------------------------------------------

class GLTFTAG_PresetItem(PropertyGroup):


    body_type: EnumProperty(
        name="Body Type",
        items=[
            ("STATIC", "Static", ""),
            ("DYNAMIC", "Dynamic", ""),
            ("KINEMATIC", "Kinematic", ""),
        ],
        default="STATIC"
    )

    collider_type: EnumProperty(
        name="Collider",
        items=[
            ("NONE", "None", ""),
            ("BOX", "Box", ""),
            ("SPHERE", "Sphere", ""),
            ("CAPSULE", "Capsule", ""),
            ("MESH", "Mesh", ""),
            ("CONVEX_HULL", "Convex Hull", ""),
        ],
        default="BOX"
    )

    misc_type: EnumProperty(
        name="Misc Tag",
        items=[
            ("NONE", "None", ""),
            ("FLOOR", "Floor", ""),
            ("LAVA", "Lava", ""),
            ("SPEED_BOOST", "Speed Boost", ""),
            ("DOUBLE_JUMP", "Double Jump", ""),
            ("DASH", "Dash", ""),
            ("TRIGGER", "Trigger", ""),
            ("PLAYER", "Player", ""),
            ("GOAL", "Goal", ""),
            ("SPAWN", "Spawn", ""),
        ],
        default="NONE"
    )




# ---------------------------------------------------
# Scene properties for current UI values
# ---------------------------------------------------

class GLTFTAG_SceneProps(PropertyGroup):
    body_type: EnumProperty(
        name="Body Type",
        items=[
            ("STATIC", "Static", ""),
            ("DYNAMIC", "Dynamic", ""),
            ("KINEMATIC", "Kinematic", ""),
        ],
        default="STATIC",
    )

    collider_type: EnumProperty(
        name="Collider",
        items=[
            ("NONE", "None", ""),
            ("BOX", "Box", ""),
            ("SPHERE", "Sphere", ""),
            ("CAPSULE", "Capsule", ""),
            ("MESH", "Mesh", ""),
            ("CONVEX_HULL", "Convex Hull", ""),
        ],
        default="BOX",
    )

    misc_type: EnumProperty(
        name="Misc Tag",
        items=[
            ("NONE", "None", ""),
            ("FLOOR", "Floor", ""),
            ("LAVA", "Lava", ""),
            ("SPEED_BOOST", "Speed Boost", ""),
            ("DOUBLE_JUMP", "Double Jump", ""),
            ("DASH", "Dash", ""),
            ("TRIGGER", "Trigger", ""),
            ("PLAYER", "Player", ""),
            ("GOAL", "Goal", ""),
            ("SPAWN", "Spawn", ""),
        ],
        default="NONE",
    )



# ---------------------------------------------------
# Operators
# ---------------------------------------------------


def apply_ui_to_object(obj, ui):
    obj["body"] = ui.body_type.lower()
    obj["collider"] = ui.collider_type.lower()
    obj["misc"] = ui.misc_type.lower()


class GLTFTAG_OT_apply_active(Operator):
    bl_idname = "gltf_tag.apply_active"
    bl_label = "Apply to Active"

    def execute(self, context):
        obj = context.active_object
        if obj is None:
            self.report({'WARNING'}, "No active object")
            return {'CANCELLED'}

        apply_ui_to_object(obj, context.scene.gltf_tag_ui)
        self.report({'INFO'}, f"Applied tags to {obj.name}")
        return {'FINISHED'}


class GLTFTAG_OT_apply_selected(Operator):
    bl_idname = "gltf_tag.apply_selected"
    bl_label = "Apply to Selected"

    def execute(self, context):
        selected = context.selected_objects
        if not selected:
            self.report({'WARNING'}, "No selected objects")
            return {'CANCELLED'}

        ui = context.scene.gltf_tag_ui
        for obj in selected:
            apply_ui_to_object(obj, ui)

        self.report({'INFO'}, f"Applied tags to {len(selected)} object(s)")
        return {'FINISHED'}


# ---------------------------------------------------
# Panel
# ---------------------------------------------------

class GLTFTAG_PT_main_panel(Panel):
    bl_label = "glTF Tag Tools"
    bl_idname = "GLTFTAG_PT_main_panel"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Game Tags"

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        ui = getattr(scene, "gltf_tag_ui", None)


        layout.prop(ui, "body_type")
        layout.prop(ui, "collider_type")
        layout.prop(ui, "misc_type")

        layout.separator()

        layout.operator("gltf_tag.apply_active", icon='OBJECT_DATA')
        layout.operator("gltf_tag.apply_selected", icon='OUTLINER_OB_GROUP_INSTANCE')


# ---------------------------------------------------
# Registration
# ---------------------------------------------------

core_classes = (
    GLTFTAG_PresetItem,
    GLTFTAG_SceneProps,
)

ui_classes = (
    GLTFTAG_OT_apply_active,
    GLTFTAG_OT_apply_selected,
    GLTFTAG_PT_main_panel,
)

classes = core_classes + ui_classes

def register():
    # Clean old registrations
    for cls in reversed(classes):
        try:
            bpy.utils.unregister_class(cls)
        except Exception:
            pass

    # Remove old scene props if they exist
    if hasattr(bpy.types.Scene, "gltf_tag_ui"):
        del bpy.types.Scene.gltf_tag_ui
    if hasattr(bpy.types.Scene, "gltf_tag_presets"):
        del bpy.types.Scene.gltf_tag_presets
    if hasattr(bpy.types.Scene, "gltf_tag_presets_index"):
        del bpy.types.Scene.gltf_tag_presets_index

    # Register property/data classes first
    for cls in core_classes:
        bpy.utils.register_class(cls)

    # Attach scene properties before UI panel can draw
    bpy.types.Scene.gltf_tag_ui = PointerProperty(type=GLTFTAG_SceneProps)
    bpy.types.Scene.gltf_tag_presets = CollectionProperty(type=GLTFTAG_PresetItem)
    bpy.types.Scene.gltf_tag_presets_index = IntProperty(default=0)

    # Register operators + panel last
    for cls in ui_classes:
        bpy.utils.register_class(cls)

def unregister():
    if hasattr(bpy.types.Scene, "gltf_tag_ui"):
        del bpy.types.Scene.gltf_tag_ui
    if hasattr(bpy.types.Scene, "gltf_tag_presets"):
        del bpy.types.Scene.gltf_tag_presets
    if hasattr(bpy.types.Scene, "gltf_tag_presets_index"):
        del bpy.types.Scene.gltf_tag_presets_index

    for cls in reversed(classes):
        try:
            bpy.utils.unregister_class(cls)
        except Exception:
            pass


if __name__ == "__main__":
    
    register()