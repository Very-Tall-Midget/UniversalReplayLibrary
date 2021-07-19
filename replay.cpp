#include "replay.h"

#include <fstream>
#include <iostream>
#include <Windows.h>
#include <algorithm>
#include <string>
#include <sstream>

URL_NS_BEGIN

bool Replay::Click::operator<(const Click& other) const
{
	if (xpos == -1 || other.xpos == -1)
		return frame < other.frame;
	else if (frame == -1 || other.frame == -1)
		return xpos < other.xpos;
	else
		return frame < other.frame && xpos < other.xpos;
}

bool Replay::Click::operator>(const Click& other) const
{
	if (xpos == -1 || other.xpos == -1)
		return frame > other.frame;
	else if (frame == -1 || other.frame == -1)
		return xpos > other.xpos;
	else
		return frame > other.frame && xpos > other.xpos;
}

bool Replay::Click::operator==(const Click& other) const
{
	return type == other.type && xpos == other.xpos && frame == other.frame;
}

Replay::Replay(float fps, ReplayType type)
	: fps(fps), type(type)
{
	clicks.clear();
}

Replay* Replay::Load(const char* path, bool* success)
{
	std::ifstream file(path);
	if (!file.fail())
	{
		file.seekg(0, std::ios::end);
		size_t length = file.tellg();
		file.seekg(0, std::ios::beg);

		char* contents = (char*)malloc(length);

		file.read(contents, length);

		return FromString(contents, length, success);
	}

	printf("Failed to open file `%s`, error code: %d\n", path, GetLastError());
	if (success) *success = false;
	return new Replay(60.0f, ReplayType::Frames);
}

Replay* Replay::FromString(const char* bytes, size_t length, bool* success)
{
	Replay* replay = new Replay;
	{
		const float* p_fps = ReCa<const float*>(bytes);
		if (!p_fps)
		{
			if (success) *success = false;
			return replay;
		}
		replay->fps = *p_fps;
	}
	{
		const ReplayType* p_type = ReCa<const ReplayType*>(bytes + 4);
		if (!p_type)
		{
			if (success) *success = false;
			return replay;
		}
		replay->type = *p_type;
	}

	size_t pos = 5;

	while (pos < length)
	{
		InputType inputType;
		{
			const InputType* p_inputType = ReCa<const InputType*>(bytes + pos++);
			if (!p_inputType)
			{
				if (success) *success = false;
				return replay;
			}
			inputType = *p_inputType;
		}

		int xpos = -1, frame = -1;
		switch (replay->type)
		{
		case ReplayType::XPos:
		{
			const int* p_xpos = ReCa<const int*>(bytes + pos);
			if (!p_xpos)
			{
				if (success) *success = false;
				return replay;
			}
			xpos = *p_xpos;
			pos += 4;
		}
			break;
		case ReplayType::Frames:
		{
			const int* p_frame = ReCa<const int*>(bytes + pos);
			if (!p_frame)
			{
				if (success) *success = false;
				return replay;
			}
			frame = *p_frame;
			pos += 4;
		}
			break;
		case ReplayType::Both:
		{
			const int* p_xpos = ReCa<const int*>(bytes + pos);
			if (!p_xpos)
			{
				if (success) *success = false;
				return replay;
			}
			xpos = *p_xpos;
			pos += 4;
		}
		{
			const int* p_frame = ReCa<const int*>(bytes + pos);
			if (!p_frame)
			{
				if (success) *success = false;
				return replay;
			}
			frame = *p_frame;
			pos += 4;
		}
			break;
		}

		replay->AddClick({ inputType, xpos, frame });
	}

	if (success) *success = true;
	return replay;
}

void Replay::AddClick(const Click& click)
{
	clicks.push_back(click);
}

void Replay::InsertClick(size_t position, const Click& click)
{
	clicks.insert(clicks.begin() + position, click);
}

void Replay::Reset(int xpos, int frame, bool playing)
{
	if (playing)
	{
		if (xpos == 0 && frame == 0) currentSearch = 0;
		else
		{
			// While current click is in the past, move back one
			while (currentSearch > 0 && ((type == ReplayType::Frames || clicks[currentSearch].xpos == -1) ? true : clicks[currentSearch].xpos >= xpos) &&
				((type == ReplayType::XPos || clicks[currentSearch].frame == -1) ? true : clicks[currentSearch].frame >= frame))
				currentSearch--;
		}
	}
	else
	{
		auto shouldRemove = [&](const Click& click)
		{
			return ((type == ReplayType::Frames || click.xpos == -1) ? true : click.xpos >= xpos) &&
				((type == ReplayType::XPos || click.frame == -1) ? true : click.frame >= frame);
		};
		clicks.erase(std::remove_if(clicks.begin(), clicks.end(), shouldRemove), clicks.end());
	}
}

void Replay::ForAllCurrentClicks(int xpos, int frame, std::function<void(Replay::Click&)> func)
{
	Click& click = GetCurrentClick(xpos, frame);
	while (click.type != InputType::None)
	{
		func(click);
		click = GetCurrentClick(xpos, frame);
	}
}

Replay::Click& Replay::GetCurrentClick(int xpos, int frame)
{
	Click empty = { InputType::None, -1, -1 };

	if (currentSearch >= clicks.size()) return empty;

	if ((type != ReplayType::Frames && clicks[currentSearch].xpos != -1 &&
		clicks[currentSearch].xpos <= xpos) ||
		(type != ReplayType::XPos && clicks[currentSearch].frame != -1 &&
			clicks[currentSearch].frame <= frame))
		return clicks[currentSearch++];

	return empty;
}

std::vector<Replay::InputType> Replay::GetCurrentClicks(int xpos, int frame)
{
	std::vector<InputType> currentClicks;

	InputType click = GetCurrentClick(xpos, frame).type;
	while (click != InputType::None)
	{
		currentClicks.push_back(click);
		click = GetCurrentClick(xpos, frame).type;
	}

	return currentClicks;
}

Replay::Click& Replay::GetLastClick(bool player2)
{
	for (long i = currentSearch - 1; i >= 0; --i)
	{
		switch (clicks[i].type)
		{
		case InputType::Player1Down:
		case InputType::Player1Up:
			if (!player2) return clicks[i];
			break;
		case InputType::Player2Down:
		case InputType::Player2Up:
			if (player2) return clicks[i];
			break;
		}
	}

	Click out = { InputType::None, -1, -1 };
	return out;
}

Replay::Click& Replay::GetClick(size_t position)
{
	if (position >= 0 && position < clicks.size()) return clicks[position];
	Click out = { InputType::None, -1, -1 };
	return out;
}

Replay::Click& Replay::GetClick(int xpos, int frame)
{
	for (size_t i = 0; i < clicks.size(); ++i)
	{
		if ((type != ReplayType::Frames && clicks[i].xpos != -1 &&
			clicks[i].xpos <= xpos) ||
			(type != ReplayType::XPos && clicks[i].frame != -1 &&
				clicks[i].frame <= frame))
			return clicks[i];
	}
	Click out = { InputType::None, -1, -1 };
	return out;
}

void Replay::MoveClickUp(size_t click)
{
	if (click > 0 && click < clicks.size())
	{
		Click temp = clicks[click];
		clicks[click] = clicks[click - 1];
		clicks[click - 1] = temp;
	}
}

void Replay::MoveClickDown(size_t click)
{
	if (click >= 0 && click < clicks.size() - 1)
	{
		Click temp = clicks[click];
		clicks[click] = clicks[click + 1];
		clicks[click + 1] = temp;
	}
}

void Replay::DeleteClick(size_t click)
{
	if (click >= 0 && click < clicks.size())
		clicks.erase(clicks.begin() + click);
}

void Replay::Sort()
{
	std::sort(clicks.begin(), clicks.end());
}

void Replay::Merge(Replay& other, bool forcePlayer2)
{
	for (Click& click : other.clicks)
	{
		if (click.type == InputType::None) continue;
		if (forcePlayer2)
		{
			bool press = click.type == InputType::Player1Down || click.type == InputType::Player2Down;
			click.type = press ? InputType::Player2Down : InputType::Player2Up;
		}
		AddClick(click);
	}
	Sort();
}

void Replay::SetCurrentSearch(size_t currentSearch)
{
	this->currentSearch = currentSearch;
}

Replay::ReplayType Replay::GetType() const
{
	return type;
}

float Replay::GetFps() const
{
	return fps;
}

std::vector<Replay::Click> Replay::GetClicks() const
{
	return clicks;
}

size_t Replay::Size() const
{
	return clicks.size();
}

std::string Replay::ToString(size_t* expectedSize, bool* success)
{
	//			  fps, replay type, each click
	size_t size = 4  + 1      +     (clicks.size() * (1 + 4 * (type == ReplayType::Both ? 2 : 1)));
	*expectedSize = size;

	std::string str;
	str.reserve(size);

	//str.append(*ReCa<char**>(&fps), 4);
	char fpsStr[4] = {};
	memcpy_s(fpsStr, 4, &fps, 4);
	str.append(fpsStr, 4);

	str += (char)type;

	for (Click& click : clicks)
	{
		if (click.type == InputType::None) continue;
		//str.append(*ReCa<char**>(&click.type), 1);
		str += (char)click.type;
		switch (type)
		{
		case ReplayType::XPos:
		{
			//str.append(*ReCa<char**>(&click.xpos), 4);
			char xposStr[4] = {};
			memcpy_s(xposStr, 4, &click.xpos, 4);
			str.append(xposStr, 4);
		} break;
		case ReplayType::Frames:
		{
			//str.append(*ReCa<char**>(&click.frame), 4);
			char frameStr[4] = {};
			memcpy_s(frameStr, 4, &click.frame, 4);
			str.append(frameStr, 4);
		} break;
		case ReplayType::Both:
		{
			//str.append(*ReCa<char**>(&click.xpos), 4);
			//str.append(*ReCa<char**>(&click.frame), 4);
			char xposStr[4] = {};
			memcpy_s(xposStr, 4, &click.xpos, 4);
			str.append(xposStr, 4);
			char frameStr[4] = {};
			memcpy_s(frameStr, 4, &click.frame, 4);
			str.append(frameStr, 4);
		} break;
		}
	}

	if (success)
	{
		*success = str.size() == size;
		if (!*success) printf("Final size should be %d, got %d\n", size, str.size());
	}
	return str;
}

bool Replay::Save(const char* path)
{
	std::ofstream file;
	file.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
	if (!file.fail())
	{
		size_t size;
		bool success = true;
		std::string str = ToString(&size, &success);
		if (success)
		{
			file.write(str.c_str(), size);
			file.close();
			return true;
		}

		printf("Failed to serialise replay\n");
		if (file.is_open()) file.close();
		return false;
	}

	printf("Failed to open file `%s`, error code: %d\n", path, GetLastError());
	return false;
}

URL_NS_END
