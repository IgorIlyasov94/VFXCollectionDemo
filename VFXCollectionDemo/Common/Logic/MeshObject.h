#pragma once

#include "../../Includes.h"
#include "IDrawable.h"

namespace Common::Logic
{
	class MeshObject : public IDrawable
	{
	public:
		MeshObject();
		~MeshObject() override;

	private:
	};
}
