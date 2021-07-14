#include <replay.h>

using namespace URL;

#define PRINT_ALL_CLICKS(replay) for (const Replay::Click& click : replay.GetClicks()) \
printf("Type: %d, XPos: %d, Frame: %d\n", click.type, click.xpos, click.frame);

int main()
{
	Replay replay(60.f, Replay::ReplayType::Frames);
	replay.AddClick({ Replay::InputType::Player1Down, -1, 20 });
	replay.AddClick({ Replay::InputType::Player1Up, -1, 40 });
	replay.AddClick({ Replay::InputType::Player1Down, -1, 60 });
	replay.AddClick({ Replay::InputType::Player1Up, -1, 80 });

	printf("Before insert:\n");
	PRINT_ALL_CLICKS(replay);
	replay.InsertClick(2, { Replay::InputType::Player1Down, -1, 100 });
	replay.InsertClick(3, { Replay::InputType::Player1Up, -1, 110 });
	printf("\nAfter insert:\n");
	PRINT_ALL_CLICKS(replay);

	printf("\nSorted:\n");
	replay.Sort();
	PRINT_ALL_CLICKS(replay);

	printf("\nReset from frame 90, recording mode:\n");
	replay.Reset(-1, 90);
	PRINT_ALL_CLICKS(replay);

	printf("\nOriginal:\n");
	PRINT_ALL_CLICKS(replay);
	
	if (replay.Save("test.replay"))
	{
		Replay newReplay = Replay::Load("test.replay");
		printf("\nNew:\n");
		PRINT_ALL_CLICKS(newReplay);

		printf("\nUpdate on frame 70:\n");
		newReplay.ForAllCurrentClicks(-1, 70, [](Replay::Click& click) {
			printf("Type: %d, XPos: %d, Frame: %d\n", click.type, click.xpos, click.frame);
		});

		printf("\nUpdate on frame 120:\n");
		newReplay.ForAllCurrentClicks(-1, 120, [](Replay::Click& click) {
			printf("Type: %d, XPos: %d, Frame: %d\n", click.type, click.xpos, click.frame);
		});
	}
	else
		printf("Failed to save\n");

	0;
}
