#pragma once

#ifndef EN_SCENEMEMBER_HPP
#define EN_SCENEMEMBER_HPP

namespace en
{
	enum struct SceneMemberType
	{
		PointLight,
		SpotLight,
		DirLight,
		SceneObject,
		None
	};

	struct SceneMember;

	template<class T>
	concept IsSceneMemberChild = std::derived_from<T, SceneMember> || std::same_as<T, SceneMember>;

	struct SceneMember
	{
		const SceneMemberType m_Type = SceneMemberType::None;

		template<IsSceneMemberChild T>
		T* CastTo()
		{
			return reinterpret_cast<T*>(this);
		}
	};
}

#endif