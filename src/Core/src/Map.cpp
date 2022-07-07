#include "Map.h"
#include "Cell.h"

using namespace AntBattle;

Map::Map(uint32_t width, uint32_t height) : m_size(width, height)
{
	m_map.resize(m_size.x() * m_size.y());

	Position pos;
	for (auto& cell : m_map) {
		cell = std::make_shared<Cell>(pos);
		incPosition(pos);
	}
}

void Map::incPosition(Position& pos, uint32_t x) const
{
	pos.addX(x);

	if (pos.x() >= m_size.x()) {
		pos.addY(pos.x() - m_size.x() + 1);
		pos.setX(m_size.x() - 1);
	}
}

uint32_t Map::absPosition(Position& pos) const
{
	return pos.x() + pos.y() * m_size.x();
}

std::weak_ptr<Cell> Map::cell(Position pos) const
{
	uint32_t idx = absPosition(pos);

	return (idx < m_map.size()) ? m_map[idx] : nullptr;
}

std::weak_ptr<Cell> Map::cell(uint32_t x, uint32_t y) const
{
	return cell(Position(x, y));
}

const Position& Map::size() const
{
	return m_size;
}

uint32_t Map::sizeX() const
{
	return m_size.x();
}

uint32_t Map::sizeY() const
{
	return m_size.y();
}