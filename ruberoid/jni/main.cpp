#include <android_native_app_glue.h>

int main(int argc, const char **argv);

void android_main(android_app *state)
{
    app_dummy();

	const char *argv[] =
	{
		"",
		"/sdcard/Download/shootout/big.json",
		"/sdcard/Download/shootout/data.json",
		"/sdcard/Download/shootout/monster.json",
	};
	main(sizeof(argv) / sizeof(argv[1]), argv);

	while (1)
	{
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;
		while ((ident = ALooper_pollAll(-1, NULL, &events, (void **)&source)) >= 0)
		{
			if (source != NULL)
			{
				source->process(state, source);
			}

			if (state->destroyRequested != 0)
			{
				return;
			}
		}
	}
}
