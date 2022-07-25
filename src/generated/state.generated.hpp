#pragma once

// DO NOT EDIT THIS FILE! It is automatically generated by tools/meta.go

static void register_generated_reflection() {
	entt::meta<vec2f>()
		.data<&vec2f::x>("x"_hs)
			.prop("display_name"_hs, "X"sv)
		.data<&vec2f::y>("y"_hs)
			.prop("display_name"_hs, "Y"sv);
	entt::meta<vec3f>()
		.data<&vec3f::x>("x"_hs)
			.prop("display_name"_hs, "X"sv)
		.data<&vec3f::y>("y"_hs)
			.prop("display_name"_hs, "Y"sv)
		.data<&vec3f::z>("z"_hs)
			.prop("display_name"_hs, "Z"sv);
	entt::meta<vec4f>()
		.data<&vec4f::x>("x"_hs)
			.prop("display_name"_hs, "X"sv)
		.data<&vec4f::y>("y"_hs)
			.prop("display_name"_hs, "Y"sv)
		.data<&vec4f::z>("z"_hs)
			.prop("display_name"_hs, "Z"sv)
		.data<&vec4f::w>("w"_hs)
			.prop("display_name"_hs, "W"sv);
	entt::meta<Player>()
		.data<&Player::possessionId>("possessionId"_hs)
			.prop("display_name"_hs, "PossessionId"sv);
	entt::meta<MoveStats>()
		.data<&MoveStats::wishDir>("wishDir"_hs)
			.prop("display_name"_hs, "WishDir"sv)
		.data<&MoveStats::wishSpeed>("wishSpeed"_hs)
			.prop("display_name"_hs, "WishSpeed"sv)
		.data<&MoveStats::lateralSpeed>("lateralSpeed"_hs)
			.prop("display_name"_hs, "LateralSpeed"sv);
	entt::meta<GroundedPlayerMove>()
		.data<&GroundedPlayerMove::gravity>("gravity"_hs)
			.prop("display_name"_hs, "Gravity"sv)
		.data<&GroundedPlayerMove::walkSpeed>("walkSpeed"_hs)
			.prop("display_name"_hs, "WalkSpeed"sv)
		.data<&GroundedPlayerMove::runSpeed>("runSpeed"_hs)
			.prop("display_name"_hs, "RunSpeed"sv)
		.data<&GroundedPlayerMove::fwdSpeed>("fwdSpeed"_hs)
			.prop("display_name"_hs, "FwdSpeed"sv)
		.data<&GroundedPlayerMove::sideSpeed>("sideSpeed"_hs)
			.prop("display_name"_hs, "SideSpeed"sv)
		.data<&GroundedPlayerMove::airSpeedCap>("airSpeedCap"_hs)
			.prop("display_name"_hs, "AirSpeedCap"sv)
		.data<&GroundedPlayerMove::airAccel>("airAccel"_hs)
			.prop("display_name"_hs, "AirAccel"sv)
		.data<&GroundedPlayerMove::maxAirSpeed>("maxAirSpeed"_hs)
			.prop("display_name"_hs, "MaxAirSpeed"sv)
		.data<&GroundedPlayerMove::accel>("accel"_hs)
			.prop("display_name"_hs, "Accel"sv)
		.data<&GroundedPlayerMove::friction>("friction"_hs)
			.prop("display_name"_hs, "Friction"sv)
		.data<&GroundedPlayerMove::frictionCutoff>("frictionCutoff"_hs)
			.prop("display_name"_hs, "FrictionCutoff"sv)
		.data<&GroundedPlayerMove::jumpSpeed>("jumpSpeed"_hs)
			.prop("display_name"_hs, "JumpSpeed"sv)
		.data<&GroundedPlayerMove::stopSpeed>("stopSpeed"_hs)
			.prop("display_name"_hs, "StopSpeed"sv)
		.data<&GroundedPlayerMove::groundTick>("groundTick"_hs)
			.prop("display_name"_hs, "GroundTick"sv)
		.data<&GroundedPlayerMove::linVel>("linVel"_hs)
			.prop("display_name"_hs, "LinVel"sv);
	entt::meta<FlyPlayerMove>()
		.data<&FlyPlayerMove::speed>("speed"_hs)
			.prop("display_name"_hs, "Speed"sv);
	entt::meta<Input>()
		.data<&Input::cursor>("cursor"_hs)
			.prop("display_name"_hs, "Cursor"sv)
		.data<&Input::cursorDelta>("cursorDelta"_hs)
			.prop("display_name"_hs, "CursorDelta"sv)
		.data<&Input::move>("move"_hs)
			.prop("display_name"_hs, "Move"sv)
		.data<&Input::lean>("lean"_hs)
			.prop("display_name"_hs, "Lean"sv)
		.data<&Input::menu>("menu"_hs)
			.prop("display_name"_hs, "Menu"sv)
		.data<&Input::jump>("jump"_hs)
			.prop("display_name"_hs, "Jump"sv);
	entt::meta<Material>()
		.data<&Material::baseColorFactor>("baseColorFactor"_hs)
			.prop("display_name"_hs, "BaseColorFactor"sv)
		.data<&Material::emissiveFactor>("emissiveFactor"_hs)
			.prop("display_name"_hs, "EmissiveFactor"sv)
		.data<&Material::diffuseFactor>("diffuseFactor"_hs)
			.prop("display_name"_hs, "DiffuseFactor"sv)
		.data<&Material::specularFactor>("specularFactor"_hs)
			.prop("display_name"_hs, "SpecularFactor"sv)
		.data<&Material::workflow>("workflow"_hs)
			.prop("display_name"_hs, "Workflow"sv)
		.data<&Material::baseColorTextureSet>("baseColorTextureSet"_hs)
			.prop("display_name"_hs, "BaseColorTextureSet"sv)
		.data<&Material::physicalDescriptorTextureSet>("physicalDescriptorTextureSet"_hs)
			.prop("display_name"_hs, "PhysicalDescriptorTextureSet"sv)
		.data<&Material::normalTextureSet>("normalTextureSet"_hs)
			.prop("display_name"_hs, "NormalTextureSet"sv)
		.data<&Material::occlusionTextureSet>("occlusionTextureSet"_hs)
			.prop("display_name"_hs, "OcclusionTextureSet"sv)
		.data<&Material::emissiveTextureSet>("emissiveTextureSet"_hs)
			.prop("display_name"_hs, "EmissiveTextureSet"sv)
		.data<&Material::metallicFactor>("metallicFactor"_hs)
			.prop("display_name"_hs, "MetallicFactor"sv)
		.data<&Material::roughnessFactor>("roughnessFactor"_hs)
			.prop("display_name"_hs, "RoughnessFactor"sv)
		.data<&Material::alphaMask>("alphaMask"_hs)
			.prop("display_name"_hs, "AlphaMask"sv)
		.data<&Material::alphaMaskCutoff>("alphaMaskCutoff"_hs)
			.prop("display_name"_hs, "AlphaMaskCutoff"sv);

}
