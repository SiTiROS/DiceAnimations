#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#define CLEAR_SCREEN "cls"
#else
#define CLEAR_SCREEN "clear"
#endif


const int		SIZE_XY = 3;											// ������ ������ (3x3)
const int		FULLSIZE_X = 9;											// ������ ������ � ������ ��������� (5x9) - �������������� ����������
const int		FULLSIZE_Y = 5;											// ������ ������ � ������ ��������� (5x9) - ������������ ����������
const int		MAX_INDENT_X = 40;										// ������ ����� ������
const int		MAX_INDENT_Y = 20;										// ������ ����� ������
const int		FRAME_WIDTH = MAX_INDENT_X + FULLSIZE_X + 1;			// ������ ����� (� ������ �������� ������)
const int		FRAME_HEIGHT = MAX_INDENT_Y + FULLSIZE_Y + 1;			// ������ ����� (� ������ �������� ������)

const int		diceQuantity = 3;
const float		gravity = 2;											// ��������� ���������� ������� (� ��������/����^2)
const int		damping = 8;											// ������������� ��� ������� (�������� �� 10, �.�. 0.8)
const float		friction = 8;											// ������ (�������� �� 10, �.�. 0.9)
const float		stop_threshold = 1.1;									// ����� ��������� (���� �������� ������, �������, ��� ����� �����������)
const int		time_step = 6;											// ������������� ��� ������� ��� ������ (� �������������)
const int		roll_duration_ms = 1000;								// ����� ������������ �������� (� �������������)
const int		num_frames = roll_duration_ms / time_step;				// ���������� ������ ��������


// ������ ������ ������. ������ ����� - 3x3 �������.
const int dicePatterns[6][SIZE_XY][SIZE_XY] =
{
	{
		{ 0, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 0 }
	}, // 1
	{
		{ 1, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 1 }
	}, // 2
	{
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
	}, // 3
	{
		{ 1, 0, 1 },
		{ 0, 0, 0 },
		{ 1, 0, 1 }
	}, // 4
	{
		{ 1, 0, 1 },
		{ 0, 1, 0 },
		{ 1, 0, 1 }
	}, // 5
	{
		{ 1, 1, 1 },
		{ 0, 0, 0 },
		{ 1, 1, 1 }
	}  // 6
};

// ��������� ��� �������� ���������
struct Location
{
	int x;
	int y;
};

// ��������� ��� �������� ��������� ������ (���������� � ��������)
struct Dice
{
	int x;					// �������������� ���������� (������ �����)
	int y;					// ������������ ���������� (������ ������)
	float vx;				// �������������� ��������
	float vy;				// ������������ ��������
	int face;				// ������� ������
};


// ������� ��� ���������� ������� ������
void CreateDice(char(&Dicearray)[FULLSIZE_Y][FULLSIZE_X], int face)
{
	// +-------+
	// | * * * |
	// | * * * |
	// | * * * |
	// +-------+

	// ������� ������, �������� ���������
	for (int i = 0; i < FULLSIZE_Y; ++i)
	{
		for (int j = 0; j < FULLSIZE_X; ++j)
		{
			Dicearray[i][j] = ' ';
		}
	}

	// ����� ������
	const char* border = "+-------+";
	for (int j = 0; j < FULLSIZE_X; ++j)
	{
		Dicearray[0][j] = border[j];							// ������� �������
		Dicearray[FULLSIZE_Y - 1][j] = border[j];				// ������ �������
	}

	// ��������� ����������
	for (int i = 0; i < SIZE_XY; ++i)
	{
		for (int j = 0; j < SIZE_XY; ++j)
		{
			Dicearray[i + 1][0] = '|';							// ����� �����
			Dicearray[i + 1][FULLSIZE_X - 1] = '|';				// ������ �����

			if (dicePatterns[face - 1][i][j] == 1)
			{
				int x = i + 1;									// �������� �� X
				int y = 2 + j * 2;								// �������� �� Y
				Dicearray[x][y] = '*';
			}
		}
	}
}


// ������� ��� ����������� ����� �������� ���� � ������� ������
void DisplayFrame(Dice dice[], size_t dicesSize)
{
	std::vector<std::string> frame(FRAME_HEIGHT);	// ������� ������ ����� - ��������� ������ ������ ���� ��������������� �������

	// ��������� ���� � ��� ������� �����
	for (int i = 0; i < FRAME_WIDTH; ++i)
	{
		frame[0] += "-";							// ����� ������� ������
	}
	for (int i = 0; i < FRAME_WIDTH; ++i)
	{
		frame[FRAME_HEIGHT - 1] += "-";				// ����� ������ ������
	}


	// ��������� ������� ������� ����� � ������� ������
	for (int i = 1; i < FRAME_HEIGHT - 1; ++i)		// ��� �� �����������
	{
		frame[i] += "|";							// ����� �������
		for (int j = 1; j < FRAME_WIDTH - 1; ++j)	// ��� �� ���������
		{
			frame[i] += " ";						// ������� ������ �����
		}
		frame[i] += "|";							// ������ �������
	}


	for (int diceNum = 0; diceNum < dicesSize; diceNum++)
	{
		// �������� �����
		char Dice[FULLSIZE_Y][FULLSIZE_X];
		CreateDice(Dice, dice[diceNum].face);

		// ��������� ����� � �����
		for (size_t i = 0; i < FULLSIZE_Y; ++i)
		{
			for (size_t j = 0; j < FULLSIZE_X; ++j)
			{
				frame[dice[diceNum].y + i][dice[diceNum].x + j] = Dice[i][j];
			}
		}
	}


	// ������� ����� � ������� �� �����
	for (const std::string& row : frame)
	{
		std::cout << row << std::endl;
	}
}


// ����������� ������������
bool IsColliding(const Dice& a, const Dice& b)
{
	return (a.x < b.x + FULLSIZE_X + 1 && a.x + FULLSIZE_X + 1 > b.x &&
		a.y < b.y + FULLSIZE_Y + 1 && a.y + FULLSIZE_Y + 1 > b.y);
}

// ��������� ��������
void StopSpeed(Dice& dice, int stop_threshold, bool typeSpeed)
{
	if (typeSpeed = true)
	{
		if (abs(dice.vx) < stop_threshold)
		{
			dice.vx = 0;
		}
	}
	else
	{
		if (abs(dice.vy) < stop_threshold)
		{
			dice.vy = 0;
		}
	}
}


void ResolveOverlap(Dice& a, Dice& b)
{
	int overlapX = std::min(a.x + FULLSIZE_X, b.x + FULLSIZE_X) - std::max(a.x, b.x);
	int overlapY = std::min(a.y + FULLSIZE_Y, b.y + FULLSIZE_Y) - std::max(a.y, b.y);

	if (overlapX < overlapY) // �������� �� X
	{
		if (a.x < b.x)
			a.x -= overlapX / 2, b.x += overlapX / 2;
		else
			a.x += overlapX / 2, b.x -= overlapX / 2;
		a.vx = -a.vx;
		b.vx = -b.vx;
	}
	else // �������� �� Y
	{
		if (a.y < b.y)
			a.y -= overlapY / 2, b.y += overlapY / 2;
		else
			a.y += overlapY / 2, b.y -= overlapY / 2;
		a.vy = -a.vy;
		b.vy = -b.vy;
	}
}


void ElasticCollision(Dice& a, Dice& b)
{
	float tempVx = 1.01;
	float tempVy = 0.5;

	a.vx *= tempVx;
	a.vy *= tempVy;
	b.vx *= tempVx;
	b.vy *= tempVy;


}



// ������������ ������� �� ������
void KeepInsideBounds(Dice diceArray[], size_t dicesSize)
{
	for (size_t currentDice = 0; currentDice < dicesSize; currentDice++)
	{

		if (diceArray[currentDice].x <= 0)									// ���� ����� ���������� � ����� ��������
		{
			diceArray[currentDice].x = 0;
			diceArray[currentDice].vx = -diceArray[currentDice].vx * (damping + 2) / 10;

			StopSpeed(diceArray[currentDice], stop_threshold, true);
		}
		else if (diceArray[currentDice].x >= MAX_INDENT_X)					// ���� ����� ���������� � ������ ��������
		{
			diceArray[currentDice].x = MAX_INDENT_X;
			diceArray[currentDice].vx = -diceArray[currentDice].vx * (damping + 2) / 10;

			StopSpeed(diceArray[currentDice], stop_threshold, true);
		}

		if (diceArray[currentDice].y <= 0)									// ���� ����� ���������� � ������� ��������
		{
			diceArray[currentDice].y = 0;
			diceArray[currentDice].vy = -diceArray[currentDice].vy * damping / 10;

			StopSpeed(diceArray[currentDice], stop_threshold, false);
		}
		else if (diceArray[currentDice].y >= MAX_INDENT_Y)					// ���� ����� ���������� � ������ ��������
		{
			diceArray[currentDice].y = MAX_INDENT_Y;
			diceArray[currentDice].vy = -diceArray[currentDice].vy * (damping - 0.1) / 10;
			diceArray[currentDice].vx = diceArray[currentDice].vx * friction / 10;

			StopSpeed(diceArray[currentDice], stop_threshold, false);
		}
	}
}


int GenerateRandomSize(int i)
{
	static bool initialized = false;
	if (!initialized)
	{
		srand(time(0));
		initialized = true;
	}

	return rand() % i + 1;
}

// ��������� ����� ��� ��������
void GenerateDiceFaces(Dice diceArray[], size_t dicesSize)
{
	for (size_t i = 0; i < dicesSize; i++)
	{
		diceArray[i].face = GenerateRandomSize(6);
	}
}


bool StopAnimation(Dice diceArray[], size_t dicesSize)
{
	std::vector<bool> tempSpeeds(dicesSize);

	for (size_t currentDice = 0; currentDice < dicesSize; currentDice++)
	{
		tempSpeeds[currentDice] = false;
		//if (diceArray[currentDice].y >= MAX_INDENT_Y && abs(diceArray[currentDice].vx) <= stop_threshold && abs(diceArray[currentDice].vy) <= stop_threshold)
		if (abs(diceArray[currentDice].vx) <= stop_threshold && abs(diceArray[currentDice].vy) <= stop_threshold)
		{
			tempSpeeds[currentDice] = true;
		}

		//std::cout << "vx: " << abs(diceArray[currentDice].vx) << "vy: " << abs(diceArray[currentDice].vy) << std::endl;
	}

	return std::all_of(tempSpeeds.begin(), tempSpeeds.end(), [](bool b) { return b; });
}




int main()
{
	// �������� �������
	Dice diceArray[diceQuantity];
	for (int i = 0; i < diceQuantity; i++)
	{
		diceArray[i] = { GenerateRandomSize(5), GenerateRandomSize(3), 0, 0, i + 1 };
		diceArray[i].vx = GenerateRandomSize(21) - 10;							// ��������� �������������� �������� �� -10 �� 10
		diceArray[i].vy = -GenerateRandomSize(6) - 5;							// ��������� ������������ �������� �� -10 �� -5 (�������������, �.�. ����� �������������� �����)
	}

	// ����� ������ ���������� ������
	for (size_t i = 0; i < diceQuantity; i++)
	{
		for (size_t j = i + 1; j < diceQuantity; j++)
		{
			if (IsColliding(diceArray[i], diceArray[j]))
			{
				diceArray[j].x = diceArray[i].x + FULLSIZE_X + 1;
			}
		}
	}

	// ���� �������� � ���������� ������������
	for (int frame = 0; frame < num_frames; ++frame)
	{
		system(CLEAR_SCREEN);

		for (size_t currentDice = 0; currentDice < diceQuantity; currentDice++)
		{
			// ��������� ����������
			diceArray[currentDice].vy += gravity;													// ����������� ������������ �������� �� �������� ��������� ���������� �������

			// ��������� ������� ������
			diceArray[currentDice].x += diceArray[currentDice].vx;									// �������� �������������� ����������
			diceArray[currentDice].y += diceArray[currentDice].vy;									// �������� ������������ ����������


			// ������������ �������
			for (size_t i = 0; i < diceQuantity; i++)
			{
				for (size_t j = i + 1; j < diceQuantity; j++)
				{
					if (IsColliding(diceArray[i], diceArray[j]))
					{
						ResolveOverlap(diceArray[i], diceArray[j]);
						ElasticCollision(diceArray[i], diceArray[j]);
					}
				}
			}

			// ��������� ������������ � ���������
			KeepInsideBounds(diceArray, diceQuantity);

		}

		// ������� ����� � ������
		GenerateDiceFaces(diceArray, diceQuantity);

		// ��������� �������� �����
		DisplayFrame(diceArray, diceQuantity);


		// ������� ��������� ��������: ������ �� ���� � �������� ������ � ����
		if (StopAnimation(diceArray, diceQuantity)) break;

		std::this_thread::sleep_for(std::chrono::milliseconds(time_step));	// ����� ����� �������
	}

	system(CLEAR_SCREEN);													// ������� ����� ����� ������� ����������
	DisplayFrame(diceArray, diceQuantity);									// ���������� ����� � �����������

	int result = 0;
	for (size_t i = 0; i < diceQuantity; i++)
	{
		result += diceArray[i].face;
	}
	std::cout << "Congratulation on the dices fell " << result << " points" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	return 0;
}