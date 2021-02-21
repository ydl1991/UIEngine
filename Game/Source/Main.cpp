#include <GameApp/GameApp.h>

// Run Visual Leak Detector only in Debug mode
#if defined(_DEBUG)
#include <VLD/vld.h>
#endif

// --------------------------------------- //
//	The Main Entry Point of Our Program
// --------------------------------------- //
int main(int argc, char* argv[])
{
	GameApp app;

	if (app.Init())
	{
		app.Run();
	}

	return 0;
}