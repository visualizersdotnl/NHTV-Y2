
/*
	Year 2 Direct3D 11 workshop template.
	World - Container handling simulation & rendering of the scene(s).
*/

#include "Platform.h"
#include "World.h"
#include "D3D.h"

namespace World
{
	bool Create()
	{
		return true;
	}

	void Destroy()
	{
	}

	bool Simulate() 
	{ 
		return true;  
	}

	void Render()
	{
		D3D::BeginFrame();
		{

		}
		D3D::EndFrame();
	}
}
