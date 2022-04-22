local ffi = require('ffi')
ffi.cdef [[

	struct Component_Core_Named {
		uint32_t name;
	};
	struct Component_Core_Global {

	};
	struct Component_Core_Position {
		float x;
		float y;
		float z;
	};
	struct Component_Core_Transform {
		struct {float x, y, z;} rotation;
		struct {float x, y, z;} scale;
	};
	struct Component_Core_ScriptedBehavior {
		uint32_t resource;
	};

	struct Component_graphics_Layer {
		uint8_t layer;
	};
	struct Component_graphics_Sprite {

	};
	struct Component_graphics_StaticImage {
		uint32_t image;
	};
	struct Component_graphics_Billboard {

	};
	struct Component_graphics_Model {
		uint32_t mesh;
	};
	struct Component_graphics_Material {
		struct {float r, g, b;} color;
		uint32_t albedo;
		uint32_t normal;
		uint32_t metalic;
		uint32_t roughness;
		uint32_t ambient_occlusion;
	};
	struct Component_graphics_PointLight {
		float radius;
		struct {float r, g, b;} color;
		float intensity;
	};
	struct Component_graphics_SpotLight {
		float range;
		struct {float r, g, b;} color;
		struct {float x, y, z;} direction;
		float intensity;
	};

	struct Component_physics_StaticBody {
		uint32_t shape;
		struct btRigidBody* physics_body;
	};
	struct Component_physics_DynamicBody {
		uint32_t shape;
		float mass;
		struct btRigidBody* physics_body;
	};
	struct Component_physics_KinematicBody {
		uint32_t shape;
		float mass;
		struct btRigidBody* physics_body;
	};
	struct Component_physics_CollisionSensor {
		uint8_t collision_mask;
	};
	struct Component_physics_TriggerRegion {
		uint32_t shape;
		uint32_t on_enter;
		uint32_t on_exit;
		uint8_t trigger_mask;
	};

]]
local core = require('mm_core')
core:register_components({
	["named"] = "struct Component_Core_Named*",
	["global"] = "struct Component_Core_Global*",
	["position"] = "struct Component_Core_Position*",
	["transform"] = "struct Component_Core_Transform*",
	["scripted-behavior"] = "struct Component_Core_ScriptedBehavior*",
	["graphics/layer"] = "struct Component_graphics_Layer*",
	["graphics/sprite"] = "struct Component_graphics_Sprite*",
	["graphics/static-image"] = "struct Component_graphics_StaticImage*",
	["graphics/billboard"] = "struct Component_graphics_Billboard*",
	["graphics/model"] = "struct Component_graphics_Model*",
	["graphics/material"] = "struct Component_graphics_Material*",
	["graphics/point-light"] = "struct Component_graphics_PointLight*",
	["graphics/spot-light"] = "struct Component_graphics_SpotLight*",
	["physics/static-body"] = "struct Component_physics_StaticBody*",
	["physics/dynamic-body"] = "struct Component_physics_DynamicBody*",
	["physics/kinematic-body"] = "struct Component_physics_KinematicBody*",
	["physics/collision-sensor"] = "struct Component_physics_CollisionSensor*",
	["physics/trigger-region"] = "struct Component_physics_TriggerRegion*",
})