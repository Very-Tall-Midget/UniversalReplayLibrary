#ifndef __URL_REPLAY_H__
#define __URL_REPLAY_H__

#include <stdint.h>
#include <vector>
#include <string>
#include <functional>

#include "macros.h"

URL_NS_BEGIN

class Replay
{
public:
	enum class InputType : int8_t
	{
		None = -1,
		Player1Down = 1, Player1Up = 0,
		Player2Down = 3, Player2Up = 2
	};

	struct Click
	{
		InputType type = InputType::None;
		int xpos = -1, frame = -1;

		bool operator<(const Click& other) const;
		bool operator>(const Click& other) const;
		bool operator==(const Click& other) const;
	};

	enum class ReplayType : uint8_t
	{
		XPos, Frames, Both
	};
private:
	float fps = 0.0f;
	ReplayType type = ReplayType::Frames;
	size_t currentSearch = 0;

	std::vector<Click> clicks;
public:
	Replay() = default;
	explicit Replay(float fps, ReplayType type);

	static Replay* Load(const wchar_t* path, bool* success=nullptr);
	static Replay* FromString(const char* bytes, size_t length, bool* success=nullptr);

	void AddClick(const Click& click);
	void InsertClick(size_t position, const Click& click);

	void Reset(int xpos=0, int frame=0, bool playing=false);
	void Finalise();

	void ForAllCurrentClicks(int xpos, int frame, std::function<void(Click&)> func);
	Click& GetCurrentClick(int xpos, int frame);
	std::vector<InputType> GetCurrentClicks(int xpos, int frame);
	Click& GetLastClick(bool player2=false);
	Click& GetClick(size_t position);
	Click& GetClick(int xpos, int frame);

	void MoveClickUp(size_t click);
	void MoveClickDown(size_t click);
	void DeleteClick(size_t click);
	void Sort();
	void Merge(Replay& other, bool forcePlayer2=false);
	void Clean();

	void SetCurrentSearch(size_t currentSearch);

	ReplayType GetType() const;
	float GetFps() const;
	std::vector<Click>& GetClicks();
	const std::vector<Click>& GetClicks() const;
	size_t GetCurrentSearch() const;
	size_t Size() const;

	std::string ToString(size_t* expectedSize=nullptr, bool* success=nullptr);
	bool Save(const wchar_t* path);
};

URL_NS_END

#endif // __URL_REPLAY_H__
