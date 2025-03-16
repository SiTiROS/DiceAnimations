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


const int		SIZE_XY = 3;											// Размер кубика (3x3)
const int		FULLSIZE_X = 9;											// Размер кубика с учётом окантовки (5x9) - Горизонтальная координата
const int		FULLSIZE_Y = 5;											// Размер кубика с учётом окантовки (5x9) - Вертикальная координата
const int		MAX_INDENT_X = 40;										// Ширина рамки экрана
const int		MAX_INDENT_Y = 20;										// Высота рамки экрана
const int		FRAME_WIDTH = MAX_INDENT_X + FULLSIZE_X + 1;			// Ширина рамки (с учетом отступов кубика)
const int		FRAME_HEIGHT = MAX_INDENT_Y + FULLSIZE_Y + 1;			// Высота рамки (с учетом отступов кубика)

const int		diceQuantity = 3;
const float		gravity = 2;											// Ускорение свободного падения (в пикселях/кадр^2)
const int		damping = 8;											// Демпфирование при отскоке (делитель на 10, т.е. 0.8)
const float		friction = 8;											// Трение (делитель на 10, т.е. 0.9)
const float		stop_threshold = 1.1;									// Порог остановки (если скорость меньше, считаем, что кубик остановился)
const int		time_step = 6;											// Фиксированный шаг времени для физики (в миллисекундах)
const int		roll_duration_ms = 1000;								// Общая длительность анимации (в миллисекундах)
const int		num_frames = roll_duration_ms / time_step;				// Количество кадров анимации


// Массив граней кубика. Каждая грань - 3x3 матрица.
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

// Структура для хранения координат
struct Location
{
	int x;
	int y;
};

// Структура для хранения состояния кубика (координаты и скорости)
struct Dice
{
	int x;					// Горизонтальная координата (отступ слева)
	int y;					// Вертикальная координата (отступ сверху)
	float vx;				// Горизонтальная скорость
	float vy;				// Вертикальная скорость
	int face;				// Сторона кубика
};


// Функция для заполнения массива кубика
void CreateDice(char(&Dicearray)[FULLSIZE_Y][FULLSIZE_X], int face)
{
	// +-------+
	// | * * * |
	// | * * * |
	// | * * * |
	// +-------+

	// Очищаем массив, заполняя пробелами
	for (int i = 0; i < FULLSIZE_Y; ++i)
	{
		for (int j = 0; j < FULLSIZE_X; ++j)
		{
			Dicearray[i][j] = ' ';
		}
	}

	// Рамка кубика
	const char* border = "+-------+";
	for (int j = 0; j < FULLSIZE_X; ++j)
	{
		Dicearray[0][j] = border[j];							// Верхняя граница
		Dicearray[FULLSIZE_Y - 1][j] = border[j];				// Нижняя граница
	}

	// Заполняем звёздочками
	for (int i = 0; i < SIZE_XY; ++i)
	{
		for (int j = 0; j < SIZE_XY; ++j)
		{
			Dicearray[i + 1][0] = '|';							// Левая грань
			Dicearray[i + 1][FULLSIZE_X - 1] = '|';				// Правая грань

			if (dicePatterns[face - 1][i][j] == 1)
			{
				int x = i + 1;									// Смещение по X
				int y = 2 + j * 2;								// Смещение по Y
				Dicearray[x][y] = '*';
			}
		}
	}
}


// Функция для отображения всего игрового поля с кубиком внутри
void DisplayFrame(Dice dice[], size_t dicesSize)
{
	std::vector<std::string> frame(FRAME_HEIGHT);	// Создаем вектор строк - заполняем строки сверху вниз горизонтальными линиями

	// Заполняем верх и ниж границы рамки
	for (int i = 0; i < FRAME_WIDTH; ++i)
	{
		frame[0] += "-";							// Самая верхняя строка
	}
	for (int i = 0; i < FRAME_WIDTH; ++i)
	{
		frame[FRAME_HEIGHT - 1] += "-";				// Самая нижняя строка
	}


	// Заполняем боковые границы рамки и пробелы внутри
	for (int i = 1; i < FRAME_HEIGHT - 1; ++i)		// Идём по горизонтали
	{
		frame[i] += "|";							// Левая граница
		for (int j = 1; j < FRAME_WIDTH - 1; ++j)	// Идём по вертикали
		{
			frame[i] += " ";						// Пробелы внутри рамки
		}
		frame[i] += "|";							// Правая граница
	}


	for (int diceNum = 0; diceNum < dicesSize; diceNum++)
	{
		// Получаем кубик
		char Dice[FULLSIZE_Y][FULLSIZE_X];
		CreateDice(Dice, dice[diceNum].face);

		// Вставляем кубик в рамку
		for (size_t i = 0; i < FULLSIZE_Y; ++i)
		{
			for (size_t j = 0; j < FULLSIZE_X; ++j)
			{
				frame[dice[diceNum].y + i][dice[diceNum].x + j] = Dice[i][j];
			}
		}
	}


	// Выводим рамку с кубиком на экран
	for (const std::string& row : frame)
	{
		std::cout << row << std::endl;
	}
}


// Обнаружение столкновений
bool IsColliding(const Dice& a, const Dice& b)
{
	return (a.x < b.x + FULLSIZE_X + 1 && a.x + FULLSIZE_X + 1 > b.x &&
		a.y < b.y + FULLSIZE_Y + 1 && a.y + FULLSIZE_Y + 1 > b.y);
}

// Остановка скорости
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

	if (overlapX < overlapY) // Смещение по X
	{
		if (a.x < b.x)
			a.x -= overlapX / 2, b.x += overlapX / 2;
		else
			a.x += overlapX / 2, b.x -= overlapX / 2;
		a.vx = -a.vx;
		b.vx = -b.vx;
	}
	else // Смещение по Y
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



// Отталкивание кубиков от стенок
void KeepInsideBounds(Dice diceArray[], size_t dicesSize)
{
	for (size_t currentDice = 0; currentDice < dicesSize; currentDice++)
	{

		if (diceArray[currentDice].x <= 0)									// Если кубик столкнулся с левой границей
		{
			diceArray[currentDice].x = 0;
			diceArray[currentDice].vx = -diceArray[currentDice].vx * (damping + 2) / 10;

			StopSpeed(diceArray[currentDice], stop_threshold, true);
		}
		else if (diceArray[currentDice].x >= MAX_INDENT_X)					// Если кубик столкнулся с правой границей
		{
			diceArray[currentDice].x = MAX_INDENT_X;
			diceArray[currentDice].vx = -diceArray[currentDice].vx * (damping + 2) / 10;

			StopSpeed(diceArray[currentDice], stop_threshold, true);
		}

		if (diceArray[currentDice].y <= 0)									// Если кубик столкнулся с верхней границей
		{
			diceArray[currentDice].y = 0;
			diceArray[currentDice].vy = -diceArray[currentDice].vy * damping / 10;

			StopSpeed(diceArray[currentDice], stop_threshold, false);
		}
		else if (diceArray[currentDice].y >= MAX_INDENT_Y)					// Если кубик столкнулся с нижней границей
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

// Случайная грань для анимации
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
	// Создание кубиков
	Dice diceArray[diceQuantity];
	for (int i = 0; i < diceQuantity; i++)
	{
		diceArray[i] = { GenerateRandomSize(5), GenerateRandomSize(3), 0, 0, i + 1 };
		diceArray[i].vx = GenerateRandomSize(21) - 10;							// Случайная горизонтальная скорость от -10 до 10
		diceArray[i].vy = -GenerateRandomSize(6) - 5;							// Случайная вертикальная скорость от -10 до -5 (отрицательная, т.к. кубик подбрасывается вверх)
	}

	// После спавна раздвигаем кубики
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

	// Цикл движения и разрешения столкновений
	for (int frame = 0; frame < num_frames; ++frame)
	{
		system(CLEAR_SCREEN);

		for (size_t currentDice = 0; currentDice < diceQuantity; currentDice++)
		{
			// Применяем гравитацию
			diceArray[currentDice].vy += gravity;													// Увеличиваем вертикальную скорость на величину ускорения свободного падения

			// Обновляем позицию кубика
			diceArray[currentDice].x += diceArray[currentDice].vx;									// Изменяем горизонтальную координату
			diceArray[currentDice].y += diceArray[currentDice].vy;									// Изменяем вертикальную координату


			// Столкновения кубиков
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

			// Обработка столкновений с границами
			KeepInsideBounds(diceArray, diceQuantity);

		}

		// Генерим грани у кубика
		GenerateDiceFaces(diceArray, diceQuantity);

		// Отрисовка текущего кадра
		DisplayFrame(diceArray, diceQuantity);


		// Условие остановки анимации: кубики на полу и скорости близки к нулю
		if (StopAnimation(diceArray, diceQuantity)) break;

		std::this_thread::sleep_for(std::chrono::milliseconds(time_step));	// Пауза между кадрами
	}

	system(CLEAR_SCREEN);													// Очищаем экран перед выводом результата
	DisplayFrame(diceArray, diceQuantity);									// Отображаем кубик с результатом

	int result = 0;
	for (size_t i = 0; i < diceQuantity; i++)
	{
		result += diceArray[i].face;
	}
	std::cout << "Congratulation on the dices fell " << result << " points" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	return 0;
}