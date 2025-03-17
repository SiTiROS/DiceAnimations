#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <thread>
#include <chrono>
#include <sstream>
#include <algorithm>

const int		DICE_QUANTITY = 3;										// Кол-во кубиков
const int		SIZE_XY = 3;											// Размер кубика (3x3)
const int		FULLSIZE_X = SIZE_XY * 2 + 3;							// Размер кубика с учётом окантовки (5x9) - Горизонтальная координата
const int		FULLSIZE_Y = SIZE_XY + 2;								// Размер кубика с учётом окантовки (5x9) - Вертикальная координата
const int		MAX_INDENT_X = 40;										// Ширина рамки экрана
const int		MAX_INDENT_Y = 20;										// Высота рамки экрана
const int		FRAME_WIDTH = MAX_INDENT_X + FULLSIZE_X + 1;			// Ширина рамки (с учетом отступов кубика)
const int		FRAME_HEIGHT = MAX_INDENT_Y + FULLSIZE_Y + 1;			// Высота рамки (с учетом отступов кубика)
const int		TIME_STEP = 6;											// Фиксированный шаг времени для физики (в миллисекундах)
const int		ROLL_DURATION_MS = 1000;								// Общая длительность анимации (в миллисекундах)
const int		NUM_FRAMES = ROLL_DURATION_MS / TIME_STEP;				// Количество кадров анимации
const float		GRAVITY = 1.5;											// Ускорение свободного падения (в пикселях/кадр^2)
const float		DAMPING = 8;											// Демпфирование при отскоке (делитель на 10, т.е. 0.8)
const float		FRICTION = 7;											// Трение (делитель на 10, т.е. 0.9)
const float		STOP_THRESHOLD = 0.7;									// Порог остановки (если скорость меньше, считаем, что кубик остановился)


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
void CreateDice(char(&Dicearray)[FULLSIZE_Y][FULLSIZE_X], const int face)
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
void DisplayFrame(const Dice dice[], const size_t dicesSize)
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
	return (a.x < b.x + FULLSIZE_X && a.x + FULLSIZE_X > b.x &&
		a.y < b.y + FULLSIZE_Y && a.y + FULLSIZE_Y > b.y);
}


// Остановка скорости
void StopSpeed(Dice& dice, const int STOP_THRESHOLD, const bool typeSpeed)
{
	if (typeSpeed == true)
	{
		if (abs(dice.vx) <= STOP_THRESHOLD)
		{
			dice.vx = 0;
		}
	}
	else
	{
		if (abs(dice.vy) <= STOP_THRESHOLD)
		{
			dice.vy = 0;
		}
	}
}


// Разрешаем коллизии
void ResolveOverlap(Dice& a, Dice& b)
{
	int overlapX = std::min(a.x + FULLSIZE_X + 1, b.x + FULLSIZE_X + 1) - std::max(a.x, b.x);
	int overlapY = std::min(a.y + FULLSIZE_Y + 1, b.y + FULLSIZE_Y + 1) - std::max(a.y, b.y);

	if (overlapX < overlapY) // Смещение по X
	{
		if (a.x < b.x)
			a.x -= overlapX / 2, b.x += overlapX / 2;
		else
			a.x += overlapX / 2, b.x -= overlapX / 2;
		a.vx = -a.vx;
		b.vx = -b.vx;
	}
	else					// Смещение по Y
	{
		if (a.y < b.y)
			a.y -= overlapY / 2, b.y += overlapY / 2;
		else
			a.y += overlapY / 2, b.y -= overlapY / 2;
		a.vy = -a.vy;
		b.vy = -b.vy;
	}
}


// Изменение скорости у кубиков при столкновении
void ElasticCollision(Dice& a, Dice& b)
{
	float tempVx = 1.1;
	float tempVy = 0.8;

	a.vx *= tempVx;
	a.vy *= tempVy;
	b.vx *= tempVx;
	b.vy *= tempVy;
}


// Отталкивание кубиков от стенок
void KeepInsideBounds(Dice diceArray[], const size_t dicesSize)
{
	for (size_t i = 0; i < dicesSize; i++)
	{

		Dice& dice = diceArray[i];							// Для краткости кода

		if (dice.x <= 0)									// Если кубик столкнулся с левой границей
		{
			dice.x = 0;
			dice.vx = -dice.vx * DAMPING / 10;

			StopSpeed(dice, STOP_THRESHOLD, true);
		}
		else if (dice.x >= MAX_INDENT_X)					// Если кубик столкнулся с правой границей
		{
			dice.x = MAX_INDENT_X;
			dice.vx = -dice.vx * DAMPING / 10;

			StopSpeed(dice, STOP_THRESHOLD, true);
		}

		if (dice.y <= 0)									// Если кубик столкнулся с верхней границей
		{
			dice.y = 0;
			dice.vy = -dice.vy * DAMPING / 10;

			StopSpeed(dice, STOP_THRESHOLD, false);
		}
		else if (dice.y >= MAX_INDENT_Y)					// Если кубик столкнулся с нижней границей
		{
			dice.y = MAX_INDENT_Y;
			dice.vy = -dice.vy * DAMPING / 10;
			dice.vx = dice.vx * FRICTION / 10;

			StopSpeed(dice, STOP_THRESHOLD, false);
		}
	}
}


// Функция для рандома
int GenerateRandomSize(const int i)
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
void GenerateDiceFaces(Dice diceArray[], const size_t dicesSize)
{
	for (size_t i = 0; i < dicesSize; i++)
	{
		diceArray[i].face = GenerateRandomSize(6);
	}
}


// Проверка на остановку анимаций
bool StopAnimation(const Dice diceArray[], const size_t dicesSize)
{
	std::vector<bool> tempSpeeds(dicesSize);

	for (size_t currentDice = 0; currentDice < dicesSize; currentDice++)
	{

		//std::cout << diceArray[currentDice].face << " VX: " << (abs(diceArray[currentDice].vx)) << std::endl;
		//std::cout << diceArray[currentDice].face << " VY: " << (abs(diceArray[currentDice].vy)) << std::endl;

		tempSpeeds[currentDice] = false;
		if (abs(diceArray[currentDice].vy) <= STOP_THRESHOLD && abs(diceArray[currentDice].vx) <= STOP_THRESHOLD)
		{
			tempSpeeds[currentDice] = true;
		}
	}

	// у всех кубиков должны быть нулевые скорости
	return std::all_of(tempSpeeds.begin(), tempSpeeds.end(), [](bool b) { return b; });
}


// Подсчёт финального кол-ва очков выпавших на кубиках
int CalculateResult(const Dice diceArray[], const size_t dicesSize)
{
	int result = 0;
	for (size_t i = 0; i < dicesSize; i++)
	{
		result += diceArray[i].face;
	}
	return result;
}


// Чистим консоль
void СlearScreen()
{
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif
}


int main()
{
	// Создание кубиков
	Dice diceArray[DICE_QUANTITY];
	for (int i = 0; i < DICE_QUANTITY; i++)
	{
		diceArray[i] = { GenerateRandomSize(5), GenerateRandomSize(3), 0, 0, i + 1 };
		diceArray[i].vx = GenerateRandomSize(21) - 10;							// Случайная горизонтальная скорость от -10 до 10
		diceArray[i].vy = -GenerateRandomSize(6) - 5;							// Случайная вертикальная скорость от -10 до -5 (отрицательная, т.к. кубик подбрасывается вверх)
	}

	// После спавна раздвигаем кубики
	for (size_t i = 0; i < DICE_QUANTITY; i++)
	{
		for (size_t j = i + 1; j < DICE_QUANTITY; j++)
		{
			if (IsColliding(diceArray[i], diceArray[j]))
			{
				diceArray[j].x = diceArray[i].x + FULLSIZE_X + 1;
			}
		}
	}

	// Цикл движения и разрешения столкновений
	for (int frame = 0; frame < NUM_FRAMES; ++frame)
	{
		СlearScreen();

		for (size_t currentDice = 0; currentDice < DICE_QUANTITY; currentDice++)
		{
			// Применяем гравитацию
			diceArray[currentDice].vy += GRAVITY;													// Увеличиваем вертикальную скорость на величину ускорения свободного падения

			// Обновляем позицию кубика
			diceArray[currentDice].x += diceArray[currentDice].vx;									// Изменяем горизонтальную координату
			diceArray[currentDice].y += diceArray[currentDice].vy;									// Изменяем вертикальную координату


			// Столкновения кубиков
			for (size_t i = 0; i < DICE_QUANTITY; i++)
			{
				for (size_t j = i + 1; j < DICE_QUANTITY; j++)
				{
					if (IsColliding(diceArray[i], diceArray[j]))
					{
						ResolveOverlap(diceArray[i], diceArray[j]);
						ElasticCollision(diceArray[i], diceArray[j]);
					}
				}
			}

			// Обработка столкновений с границами
			KeepInsideBounds(diceArray, DICE_QUANTITY);
		}

		// Генерим грани у кубика
		GenerateDiceFaces(diceArray, DICE_QUANTITY);

		// Отрисовка текущего кадра
		DisplayFrame(diceArray, DICE_QUANTITY);


		// Условие остановки анимации: скорости близки к нулю
		if (StopAnimation(diceArray, DICE_QUANTITY)) break;

		std::this_thread::sleep_for(std::chrono::milliseconds(TIME_STEP));	// Пауза между кадрами
	}

	СlearScreen();
	DisplayFrame(diceArray, DICE_QUANTITY);									// Отображаем кубики с результатом


	std::cout << "Congratulation on the dices fell " << CalculateResult(diceArray, DICE_QUANTITY) << " points" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	return 0;
}