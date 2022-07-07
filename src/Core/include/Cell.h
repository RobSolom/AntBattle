#pragma once

#include <stdint.h>
#include <memory>

#include "Position.h"

namespace AntBattle {

class Ant;

class Cell
{
public:
	Cell(const Position& pos);
	Cell(uint32_t x, uint32_t y);
	virtual ~Cell() = default;

	bool isEmpty() const { return !m_food && m_ant.expired() && !m_isStone; }
	uint32_t getFood() const { return m_food; }
	bool isStone () const { return m_isStone; }
	std::weak_ptr<Ant> getAnt() const { return m_ant; }

	void setFood(uint32_t count);
	void setAnt(const std::weak_ptr<Ant>& ant);
	void removeAnt();
	void setStone(bool isstone);

protected:
	Position m_pos;
	uint32_t m_food = 0;
	std::weak_ptr<Ant> m_ant;
	bool m_isStone = false;
};

};
