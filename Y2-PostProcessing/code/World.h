
/*
	Year 2 Direct3D 11 workshop template.
	World - Container handling simulation & rendering of the scene(s).
*/

#if !defined(WORLD_H)
#define WORLD_H

namespace World
{
	bool Create();
	void Destroy();

	// Simulate() can signal a stop to the action; if it doesn't call Render().
	bool Simulate();
	void Render();
}

#endif // WORLD_H
