#include "Battle.h"

#include "Log.h"
#include "Config.h"

#include "Ant.h"
#include "Math.h"
#include "Map.h"
#include "Cell.h"
#include "Player.h"

#include "BattleLogService.h"
#include "FileProvider.h"

using namespace AntBattle;

Battle::Battle(const std::string& confname, const std::vector<std::string>& players)
{
	m_conf = std::make_shared<Config>(confname);
	m_map  = std::make_shared<Map>(m_conf);

	// create the player list
	uint32_t player_index = 0;
	for (auto& libname : players) {
		m_players.push_back(std::make_shared<Player>(player_index++, libname));
	}

	for (auto& player : m_players) {
		if (!player->isInit()) {
			Log::instance().put(format("Player '%s' [%s] is not initiliazed", player->teamName().c_str(), player->library().c_str()));
			return;
		}
	}

	// check players names
	for (int ii = 0; ii < m_players.size() - 1; ++ii) {
		uint32_t count = 1;

		for (int jj = ii + 1; jj < m_players.size(); ++jj) {
			if (m_players[ii]->teamName() == m_players[jj]->teamName()) {
				m_players[jj]->changeTeamName(count++);
			}
		}
	}

	m_logService.add(std::make_shared<FileProvider>("test_battle.json"));

	Log::instance().put(format("create new battle. map is [%i x %i]", m_conf->width(), m_conf->height()));

	m_isInit = true;
}

void Battle::run()
{
	if (!m_isInit) {
		Log::instance().put("Can't start the battle. Battle is not initialized");
		return;
	}

	//TODO create UID of battle

	// create list of all ant
	m_ants = m_map->generate(m_players);

	// send of starting info
	for (auto& player : m_players) {
		m_logService.savePlayer(player);
	}
	m_logService.saveMap(m_map);

	// main loop
	while(true) {
		//---------------------------------------------------------------------
		// Start of iteration

		// clear IsChange flag
		m_map->clearChanged();
		m_iteration++;

		// randomize the list of ants
		std::shuffle(m_ants.begin(), m_ants.end(), Math::randGenerator());

		// save log
		m_logService.saveNewTurn(m_iteration);


		//---------------------------------------------------------------------
		// Ant phase
		for (auto& ant : m_ants) {
			auto player = ant->player().lock();

			if(!ant->hasCommand()) {
				AntInfo ai;
				Command cmd;

				auto queen = player->antQueen().lock();

				generateAntInfo(ant, ai);
				ai.iteration = m_iteration;
				ai.countOfWorker = player->countOfWorkers();
				ai.countOfSolder = player->countOfSolders();
				ai.countOfFood = queen->cargo();

				ant->process(ai, cmd);
				ant->setCommand(cmd);
			}

			doAntCommand(ant);

			//TODO decrease m_satiety, check for 0

			//TODO check health for 0
			//TODO to kill ant, update statistic
		}

		//---------------------------------------------------------------------
		// End of iteration
		m_logService.saveMap(m_map);
	}
}

void Battle::doAntCommand(AntSharedPtr& ant)
{
	Command& cmd = ant->command();

	switch(cmd.type) {
		case CommandType::Idle: cmd.type = CommandType::Explore; cmd.count = 1;
		case CommandType::Explore: commandAntExplore(ant); break;
		case CommandType::Move: commandAntMove(ant); break;
//		case CommandType::Eat: doAntEat(ant);break;
//		case CommandType::MoveToFood: doAntMoveToFood(ant); break;
//		case CommandType::Attack: doAntAttack(ant); break;
//		case CommandType::MoveAndAttack: doAntMoveAndAttack(ant); break;
//		case CommandType::Feed: doAntFeed(ant); break;
//		case CommandType::FeedQueen: doAntFeedQueen(ant); break;
//		case CommandType::Defense: doAntDefence(ant); break;
//		case CommandType::CreateOfWorker: doAntCreateWorker(ant); break;
//		case CommandType::CreateOfSolder: doAntCreateSolder(ant); break;
		default:
			cmd.type = CommandType::Explore;
			cmd.count = 1;
			commandAntExplore(ant);
			break;
	}

	if (!cmd.count) {
		ant->clearCommand();
	}
}

void Battle::commandAntExplore(AntSharedPtr& ant)
{
	std::shared_ptr<Player> player = ant->player().lock();
	std::shared_ptr<Ant> queen = player->antQueen().lock();

	Direction dirToQueen = Math::directionTo(ant->position(), queen->position());
	Direction dirFromQueen =Math::inverseDirection(dirToQueen);
	Direction dir = Math::probabilisticDirection(dirFromQueen);

	moveAnt(ant, dir);

	--ant->command().count;
}

void Battle::commandAntMove(AntSharedPtr& ant)
{
	Direction dir = Math::probabilisticDirection(ant->command().direction);

	moveAnt(ant, dir);

	--ant->command().count;
}

/// \brief Move the ant to selected direction
///
/// If we can not move the ant to selected direction then we move ant to direction near to selected
void Battle::moveAnt(AntSharedPtr& ant, const Direction& dir)
{
	// get odered directions
	std::vector<Direction> array_dir = Math::createDirectionArray(ant->command().direction);

	for (auto& dir : array_dir) {
		// get the new position as the ant position and the direction
		Position newpos = ant->position() + Math::positionOffset(dir);

		// checking reachability of the new position
		if (m_map->isCellEmpty(ant->position())) {
			// if the cell is empty then move the ant to it
			m_map->moveAnt(ant, newpos);
			return;
		}

		// cell is not empty, go to next the direction and to check next
	}

	ant->clearCommand();
}

void Battle::generateAntInfo(AntSharedPtr& ant, AntInfo& ai)
{
	auto visible = Math::visibleCells(ant->position(), ant->maxVisibility());
	auto owner = ant->player().lock();
	auto quuen = owner->antQueen().lock();
	uint32_t min_dist_food = 0xffffffff;
	uint32_t min_dist_enemy = 0xffffffff;

	ai.healthPrecent = ant->healthPercent();
	ai.satietyPrecent = ant->satietyPercent();
	ai.cargo = ant->cargo();
//	ai.directionToQueen = Math::directionTo(ant->position(), quuen->position());
	ai.distanceToQueen = Math::distanceTo(ant->position(), quuen->position());
	ai.directionToLabel = Direction::Nord; //TODO fix it
	ai.distanceToLabel = 0; //TODO fix it
	ai.directionToNearEnemy = Direction::Nord;
	ai.directionToNearFood = Direction::Nord;

	for (auto& pos : visible) {
		auto cell = m_map->cell(pos).lock();

		if (cell->isEmpty() || cell->isStone()) {
			continue;
		} else if(cell->food()) {
			uint32_t dist = Math::distanceTo(ant->position(), pos);

			if (dist < min_dist_food) {
				min_dist_food = dist;
				ai.directionToNearFood = Math::directionTo(ant->position(), pos);
			}

			ai.countOfVisibleFood++;
		} else {
			auto cell_ant = cell->ant().lock();
			auto cell_player = cell_ant->player().lock();

			if (owner->index() == cell_player->index()) {
				ai.countOfVisibleAlly++;
			} else {
				uint32_t dist = Math::distanceTo(ant->position(), pos);

				if (dist < min_dist_enemy) {
					min_dist_enemy = dist;
					ai.directionToNearEnemy = Math::directionTo(ant->position(), pos);
				}

				ai.countOfVisibleEnemies++;
			}
		}
	}
}
