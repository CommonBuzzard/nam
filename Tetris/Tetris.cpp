#include <SFML/Graphics.hpp>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <ctime>
#include <queue>
#include <vector>

using namespace sf;
using namespace std;

const int BG_MARGIN = 20;
const int FIELD_WIDTH = 10;
const int FIELD_HEIGHT = 20;
const float NORMAL_SPEED = 0.2f;
const int SPEED_UP_DIVIDER = 4;
const int QUEUE_SIZE = 10;



class Block
{
public:
	enum Type
	{
		I,
		O,
		T,
		S,
		Z,
		J,
		L,
		Count,
		None
	};

	static void Render(RenderTarget& wnd, Type type, int x, int y, int tileSize, int rota);

	static vector<char> GetBlockData(Type type, int rota);
	static vector<Vector2i> GetCollisionTiles(Type type, int rota);

	static Color GetColor(Type type);

};

class Field
{
public:
	Field(int tileSize);

	void SetPosition(int x, int y);
	void Spawn(Block::Type t);
	void PlaceBlock();
	inline queue<Block::Type> GetQueue() { return queue; }
	void Reset();

	void OnEvent(Event e);
	void OnUpdate();
	void Render(RenderTarget& wnd);

private:
	VertexBuffer grid;
	RenderStates position;
	int translateX, translateY, tileSize;
	Block::Type map[FIELD_WIDTH][FIELD_HEIGHT];

	queue<Block::Type> queue;
	Block::Type current;
	int currentRotation = 0;
	int blockOffset = 3;
	int blockHeight = -4;

	bool checkLeftBounds();
	bool checkRightBounds();
	inline Block::Type getRandomBlock() { return (Block::Type)(rand() % Block::Type::Count); }
};

class Game
{
private:
	int wndWidth;
	int wndHeight;

	Field field;
	Clock fieldClock;
	float updateSpeed = 0.20f;
	float timePassed = 0.0f;

public:
	Game(int w, int h);

	void OnEvent(Event e);
	void OnUpdate();
	void Render(RenderTarget& wnd);


};

Game::Game(int w, int h) :
	field((h - BG_MARGIN * 2) / FIELD_HEIGHT)
{
	wndWidth = w;
	wndHeight = h;
}
void Game::OnEvent(Event e)
{
	if (e.type == Event::KeyPressed) {
		Keyboard::Key kc = e.key.code;

		if (kc == Keyboard::Down || kc == Keyboard::S)
			updateSpeed = NORMAL_SPEED / SPEED_UP_DIVIDER;
	}
	else if (e.type == Event::KeyReleased) {
		Keyboard::Key kc = e.key.code;

		if (kc == Keyboard::Down || kc == Keyboard::S)
			updateSpeed = NORMAL_SPEED;
	}

	field.OnEvent(e);
}
void Game::OnUpdate()
{
	timePassed += fieldClock.restart().asSeconds();
	while (timePassed >= updateSpeed) {
		field.OnUpdate();
		timePassed -= updateSpeed;
	}
}
void Game::Render(RenderTarget& wnd)
{
	RectangleShape bg;
	bg.setFillColor(Color(30, 30, 30));

	bg.setPosition(BG_MARGIN, BG_MARGIN);
	bg.setSize(Vector2f(1.f / 3.f * (wndWidth - BG_MARGIN * 3), wndHeight - BG_MARGIN * 2));
	wnd.draw(bg);


	Vector2f qOffset = bg.getPosition();
	queue<Block::Type> blocks = field.GetQueue();
	int queueTileSize = bg.getSize().x / 6.f;
	int vertSlots = (bg.getSize().y / queueTileSize) / 5;
	for (int i = 0; i <= min<int>(vertSlots, QUEUE_SIZE); i++) {
		Block::Type type = blocks.front();
		blocks.pop();


		float multiplier = 1.f;
		if (type != Block::Type::I && type != Block::Type::O)
			multiplier = 0.5f;

		Block::Render(wnd, type, qOffset.x + queueTileSize * multiplier, qOffset.y + i * queueTileSize * 5, queueTileSize, 0);
	}


	bg.setPosition(20 + bg.getSize().x + BG_MARGIN, BG_MARGIN);
	bg.setSize(Vector2f(2.f / 3.f * (wndWidth - BG_MARGIN * 3), wndHeight - BG_MARGIN * 2));
	wnd.draw(bg);


	field.SetPosition(bg.getPosition().x, bg.getPosition().y);
	field.Render(wnd);
}

Field::Field(int ts)
{
	tileSize = ts;

	Reset();

	int actualW = FIELD_WIDTH - 1;
	int actualH = FIELD_HEIGHT - 1;
	Color gridColor = Color(50, 50, 50, 128);

	vector<Vertex> verts(actualW * 2 + actualH * 2);

	for (int x = 0; x < actualW; x++)
	{
		verts[x * 2].position = Vector2f((x + 1)*ts, 0);
		verts[x * 2 + 1].position = Vector2f((x + 1)*ts, FIELD_HEIGHT * ts);

		verts[x * 2].color = gridColor;
		verts[x * 2 + 1].color = gridColor;
	}

	int off = actualW * 2;
	for (int y = 0; y < actualH; y++)
	{
		verts[off + y * 2].position = Vector2f(0, (y + 1)*ts);
		verts[off + y * 2 + 1].position = Vector2f(FIELD_WIDTH * ts, (y + 1)*ts);

		verts[off + y * 2].color = gridColor;
		verts[off + y * 2 + 1].color = gridColor;
	}

	grid.create(verts.size());
	grid.setUsage(VertexBuffer::Usage::Static);
	grid.setPrimitiveType(PrimitiveType::Lines);
	grid.update(verts.data(), verts.size(), 0);

}
void Field::SetPosition(int x, int y)
{
	translateX = x;
	translateY = y;
	position = RenderStates();
	position.transform.translate(x, y);
}
void Field::Spawn(Block::Type t)
{
	blockOffset = 3;
	blockHeight = -4;
	currentRotation = 0;
	current = t;

	if (queue.size() != 0) {
		queue.pop();
		queue.push(getRandomBlock());
	}
}
void Field::PlaceBlock()
{
	vector<char> tiles = Block::GetBlockData(current, currentRotation);
	for (int i = 0; i < tiles.size(); i++) {
		int offsX = blockOffset + (i % 4);
		int offsY = blockHeight + (i / 4);

		if (offsX < 0 || offsX >= FIELD_WIDTH || offsY >= FIELD_HEIGHT)
			continue;

		if (tiles[i] != 0) {
			if (offsY < 0) {
				Reset();
				return;
			}
			else {
				map[offsX][offsY] = current;
			}
		}
	}

	bool deletedRows[4] = { false };
	for (int y = blockHeight; y < min<int>(FIELD_HEIGHT, blockHeight + 4); y++) {
		bool full = true;

		for (int x = 0; x < FIELD_WIDTH; x++)
			if (map[x][y] == Block::Type::None) {
				full = false;
				break;
			}

		if (full) {
			deletedRows[y - blockHeight] = true;
			for (int x = 0; x < FIELD_WIDTH; x++)
				map[x][y] = Block::Type::None;
		}
	}

	int moveOffset = 0;
	for (int y = FIELD_HEIGHT; y >= 0; y--) {
		if (y >= blockHeight && y < blockHeight + 4)
			if (deletedRows[y - blockHeight]) {
				moveOffset++;
				continue;
			}

		if (moveOffset == 0)
			continue;

		for (int x = 0; x < FIELD_WIDTH; x++)
			map[x][y + moveOffset] = map[x][y];
	}
}

void Field::Reset()
{
	for (int x = 0; x < FIELD_WIDTH; x++) {
		for (int y = 0; y < FIELD_HEIGHT; y++) {
			map[x][y] = Block::Type::None;
		}
	}
	Spawn(getRandomBlock());

	for (int i = 0; i < QUEUE_SIZE; i++) {
		queue.push(getRandomBlock());
	}
}

void Field::OnEvent(Event e)
{
	if (e.type == Event::KeyPressed) {
		Keyboard::Key kc = e.key.code;

		if (kc == Keyboard::Up || kc == Keyboard::W) {
			currentRotation++;
			if (checkLeftBounds() || checkRightBounds())
				currentRotation--;
		}
		else if (kc == Keyboard::Left || kc == Keyboard::A) {
			blockOffset = max<int>(blockOffset - 1, -2);
			if (checkLeftBounds())
				blockOffset++;
		}
		else if (kc == Keyboard::Right || kc == Keyboard::D) {
			blockOffset = min<int>(blockOffset + 1, FIELD_WIDTH - 1);
			if (checkRightBounds())
				blockOffset--;
		}
	}
}
void Field::OnUpdate()
{
	vector<Vector2i> tiles = Block::GetCollisionTiles(current, currentRotation);
	for (int i = 0; i < tiles.size(); i++) {
		int offsX = tiles[i].x + blockOffset;
		int offsY = tiles[i].y + blockHeight;

		bool shouldPlace = false;

		if (offsY == FIELD_HEIGHT)
			shouldPlace = true;

		if (offsX >= 0 && offsX < FIELD_WIDTH && offsY >= 0 && offsY < FIELD_HEIGHT)
			if (map[offsX][offsY] != Block::Type::None)
				shouldPlace = true;

		if (shouldPlace) {
			PlaceBlock();
			Spawn(queue.front());
			break;
		}
	}

	blockHeight++;
}
void Field::Render(RenderTarget & wnd)
{
	Block::Render(wnd, current, translateX + tileSize * blockOffset, translateY + tileSize * blockHeight, tileSize, currentRotation);

	RectangleShape tile;
	tile.setSize(Vector2f(tileSize, tileSize));
	for (int x = 0; x < FIELD_WIDTH; x++) {
		for (int y = 0; y < FIELD_HEIGHT; y++) {
			tile.setPosition(x*tileSize, y*tileSize);
			tile.setFillColor(Block::GetColor(map[x][y]));
			wnd.draw(tile, position);
		}
	}

	wnd.draw(grid, position);
}

bool Field::checkLeftBounds()
{
	vector<char> data = Block::GetBlockData(current, currentRotation);
	for (int i = 0; i < data.size(); i++) {
		int offsX = i % 4 + blockOffset;
		int offsY = i / 4 + blockHeight;

		if (data[i] == 0)
			continue;


		if (offsX < 0)
			return true;

		if (offsX >= 0 && offsX < FIELD_WIDTH && offsY >= 0 && offsY < FIELD_HEIGHT)
			if (map[offsX][offsY] != Block::Type::None)
				return true;
	}
	return false;
}
bool Field::checkRightBounds()
{
	vector<char> data = Block::GetBlockData(current, currentRotation);
	for (int i = 0; i < data.size(); i++) {
		int offsX = i % 4 + blockOffset;
		int offsY = i / 4 + blockHeight;

		if (data[i] == 0)
			continue;


		if (offsX >= FIELD_WIDTH)
			return true;


		if (offsX >= 0 && offsX < FIELD_WIDTH && offsY >= 0 && offsY < FIELD_HEIGHT)
			if (map[offsX][offsY] != Block::Type::None)
				return true;
	}

	return false;
}

void Block::Render(RenderTarget & wnd, Type type, int xPos, int yPos, int tileSize, int rota)
{

	vector<char> myData = GetBlockData(type, rota);

	RectangleShape tile;
	tile.setSize(Vector2f(tileSize, tileSize));
	tile.setFillColor(GetColor(type));

	for (int i = 0; i < 16; i++) {
		if (myData[i] == 0)
			continue;

		tile.setPosition(xPos + (i % 4) * tileSize, yPos + (i / 4) * tileSize);
		wnd.draw(tile);
	}
}
vector<char> Block::GetBlockData(Type type, int rota)
{
	static const int RotaCount[Type::Count] = {
		2,	// I
		1,	// O
		4,	// T
		2,	// S
		2,	// Z
		4,	// J
		4,	// L
	};


	static const vector<vector<char>> data = {
		// I
		{
			0, 0, 0, 0,
			1, 1, 1, 1,
			0, 0, 0, 0,
			0, 0, 0, 0
		},
				{
					0, 0, 1, 0,
					0, 0, 1, 0,
					0, 0, 1, 0,
					0, 0, 1, 0,
				},

				// O
				{
					0, 0, 0, 0,
					0, 1, 1, 0,
					0, 1, 1, 0,
					0, 0, 0, 0,
				},

				// T
				{
					0, 0, 0, 0,
					0, 1, 1, 1,
					0, 0, 1, 0,
					0, 0, 0, 0
				},
				{
					0, 0, 1, 0,
					0, 0, 1, 1,
					0, 0, 1, 0,
					0, 0, 0, 0
				},
				{
					0, 0, 1, 0,
					0, 1, 1, 1,
					0, 0, 0, 0,
					0, 0, 0, 0
				},
				{
					0, 0, 1, 0,
					0, 1, 1, 0,
					0, 0, 1, 0,
					0, 0, 0, 0
				},

		// S
				{
					0, 0, 0, 0,
					0, 0, 1, 1,
					0, 1, 1, 0,
					0, 0, 0, 0,
				},
				{
					0, 0, 1, 0,
					0, 0, 1, 1,
					0, 0, 0, 1,
					0, 0, 0, 0,
				},

				// Z
				{
					0, 0, 0, 0,
					0, 1, 1, 0,
					0, 0, 1, 1,
					0, 0, 0, 0,
				},
				{
					0, 0, 0, 1,
					0, 0, 1, 1,
					0, 0, 1, 0,
					0, 0, 0, 0,
				},

				// J
				{
					0, 0, 0, 0,
					0, 1, 1, 1,
					0, 0, 0, 1,
					0, 0, 0, 0
				},
				{
					0, 0, 1, 1,
					0, 0, 1, 0,
					0, 0, 1, 0,
					0, 0, 0, 0
				},
				{
					0, 1, 0, 0,
					0, 1, 1, 1,
					0, 0, 0, 0,
					0, 0, 0, 0
				},
				{
					0, 0, 1, 0,
					0, 0, 1, 0,
					0, 1, 1, 0,
					0, 0, 0, 0
				},


		// L
				{
					0, 0, 0, 0,
					0, 1, 1, 1,
					0, 1, 0, 0,
					0, 0, 0, 0
				},
				{
					0, 0, 1, 0,
					0, 0, 1, 0,
					0, 0, 1, 1,
					0, 0, 0, 0
				},
				{
					0, 0, 0, 1,
					0, 1, 1, 1,
					0, 0, 0, 0,
					0, 0, 0, 0
				},
				{
					0, 1, 1, 0,
					0, 0, 1, 0,
					0, 0, 1, 0,
					0, 0, 0, 0
				}
	};


	rota = rota % RotaCount[type];

	int offset = 0;
	for (int i = 0; i < type; i++) {
		offset += RotaCount[i];
	}


	return data[offset + rota];
}
vector<Vector2i> Block::GetCollisionTiles(Type type, int rota)
{
	vector<char> data = Block::GetBlockData(type, rota);
	vector<Vector2i> ret;

	for (int i = 0; i < 16; i++) {
		int x = i % 4;
		int y = i / 4;
		int indexBelow = (y + 1) * 4 + x;

		if (data[y * 4 + x] != 0) {
			if (indexBelow >= 16 || data[indexBelow] == 0)
				ret.push_back(Vector2i(x, y + 1));
		}
	}

	return ret;
}

Color Block::GetColor(Type type)
{
	static const Color Colors[(int)Block::Count] = {
		Color(43, 172, 225),	// I
		Color(253, 225, 0),		// O
		Color(146, 43, 140),	// T
		Color(78, 183, 72),		// S
		Color(238, 39, 51),		// Z
		Color(0, 90, 157),		// J
		Color(248, 150, 34)		// L
	};
	return Colors[type];
}



int main()
{
	srand(time(NULL));

	RenderWindow wnd(VideoMode(600, 760), "Tetris", Style::Titlebar | Style::Close);
	wnd.setFramerateLimit(30);
	Event e;

	Game game(wnd.getSize().x, wnd.getSize().y);

	while (wnd.isOpen()) {
		while (wnd.pollEvent(e)) {
			if (e.type == Event::Closed)
				wnd.close();
			game.OnEvent(e);
		}

		game.OnUpdate();

		wnd.clear();
		game.Render(wnd);
		wnd.display();
	}


	return 0;
}