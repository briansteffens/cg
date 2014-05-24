#ifndef GAME_H
#define GAME_H

#include <vector>
#include <string>

#include "common.h"
#include "comm.h"


class Skill
{
};


class Entity
{
public:
	virtual void Update() {}	
	
	inline int GetID() const { return id; }
	inline void SetID(unsigned int id) { this->id = id; }
	
	inline std::string GetName() const { return name; }
	inline void SetName(const std::string& name) { this->name = name; }
	
	inline Point& GetLoc() { return loc; }
	inline void SetLoc(Point loc) { this->loc = loc; }
private:
	unsigned int id;
	std::string name;
	Point loc;
};


class Character;


class Action
{
public:
	enum ActionDuration { DURATION_INFINITE = -1 };

	Action(Character& actor)
		: actor(actor)
	{
		started_at = Time::GetNow();
	}
	
	virtual ~Action() {}

	inline Character& GetActor() const { return actor; }
	
	inline unsigned int GetStartedAt() const { return started_at; }
	inline void SetStartedAt(unsigned int val) { this->started_at = val; }
	
	inline unsigned int GetDuration() const { return duration; }
	inline void SetDuration(unsigned int val) { this->duration = val; }
	
	inline bool IsActive()
	{
		return ((duration == DURATION_INFINITE) || 
			   (Time::GetNow() - started_at < duration)); 
	}
	
private:
	Character& actor;
	unsigned int started_at;
	unsigned int duration = 1000;
};


class IdleAction : public Action
{
	IdleAction(Character& actor)
		: Action(actor)
	{
		SetDuration(-1);
	}
};


class SkillAction : public Action
{
public:
	SkillAction(Character& actor, Skill& skill)
		: Action(actor), 
		  skill(skill) {}

	inline Skill& GetSkill() const { return skill; }

	inline std::vector<Character*> const& GetTargets() const { return targets; }
	inline std::vector<Character*>& GetTargetsMutable() { return targets; }
	
private:
	Skill& skill;
	std::vector<Character*> targets;
};


class Character : public Entity
{
public:
	virtual void Update()
	{
		unsigned int now = Time::GetNow();
				
		if (IsMoving())
		{
			unsigned int delta = now - last_move;
			
			if (delta > speed)
			{
				unsigned int remainder = delta % speed;
				unsigned int temp = delta - remainder;
				unsigned int complete_moves = temp / speed;
				
				for (unsigned int i = 0; i < complete_moves; i++)
				{
					if (path.size() == 0)
						break;
						
					SetLoc(*path.begin());
					path.erase(path.begin());
					last_move = now - remainder;
				}
			}
		}
	}
	
	void ClearPath()
	{
		path.clear();
	}
	
	void QueuePath(Point point)
	{
		// If not already moving, delay the first move
		if (!IsMoving())
			last_move = Time::GetNow();
			
		path.push_back(point);
	}
	
	Point DirectionMoving()
	{
		Point ret(0, 0);
		
		if (!IsMoving()) 
			return ret;
		
		Point next = *path.begin();
		Point loc = GetLoc();
		
		if (next.x > loc.x)
			ret.x = 1;
		else if (next.x < loc.x)
			ret.x = -1;
		
		if (next.y > loc.y)
			ret.y = 1;
		else if (next.y < loc.y)
			ret.y = -1;
		
		return ret;
	}
	
	inline int GetLastMove() { return last_move; }
	inline bool IsMoving() { return path.size() > 0; }
	inline int GetSpeed() const { return speed; }
	inline void SetSpeed(unsigned int speed) { this->speed = speed; }
	inline int GetHP() const { return hp; }
	inline void SetHP(unsigned int hp) { this->hp = hp; }
	
	Point GetPathEnd() 
	{
		if (!IsMoving())
			return GetLoc();
		
		return *(path.end() - 1);
	}
	
	inline Action const* GetAction() const { return action; }
	inline Action* GetActionMutable() { return action; }
	inline void SetAction(Action* action) { this->action = action; }
	inline void ResetAction() { this->action = NULL; }
	
	inline std::vector<Action*> const& GetAffectedBy() const 
		{ return affected_by; }
	inline std::vector<Action*>& GetAffectedByMutable() { return affected_by; }
	
protected:
private:
	std::vector<Point> path;
	unsigned int last_move;
	unsigned int speed = 125;
	unsigned int hp = 100;
	
	Action* action = NULL;
	std::vector<Action*> affected_by;
};


class GameEngine
{
public:
	GameEngine(Endpoint& endpoint)
		: endpoint(endpoint)
	{
		Time::UpdateNow();
	}
	
	virtual ~GameEngine()
	{
		for (unsigned int i = 0; i < entities.size(); i++)
			delete entities[i];
		
		for (unsigned int i = 0; i < actions.size(); i++)
			delete actions[i];
	}
	
	virtual void Update()
	{
		Time::UpdateNow();
		
		PruneActions();
		
		/*
		while (Message* msg = endpoint.Poll())
		{
			std::cout << "message received in game engine" << std::endl;
			delete msg;
		}
		*/
		for (auto &e : entities) 
			e->Update();
	}
	
	void Register(Entity* entity)
	{
		entities.push_back(entity);
	}
	
	void Deregister(Entity* entity)
	{
		unsigned int offset = 0;
		for (auto &e : entities) {
			if (e == entity)
				goto found;
			offset++;
		}
		throw std::runtime_error("Entity not found.");
	found:
		entities.erase(entities.begin() + offset);
	}
	
	Entity* GetEntityByID(unsigned int entity_id)
	{
		for (auto &e : entities)
			if (e->GetID() == entity_id)
				return e;
		
		throw std::runtime_error("Entity not found.");
	}
	
	void Register(Action& action)
	{
		std::cout << action.GetActor().GetName() << std::endl;
		action.GetActor().SetAction(&action);
		
		if (SkillAction* skill_action = dynamic_cast<SkillAction*>(&action))
			for (auto& t : skill_action->GetTargets())
				t->GetAffectedByMutable().push_back(skill_action);

		actions.push_back(&action);
	}
	
	inline Character* GetAvatar() const { return avatar; }
	inline void SetAvatar(Character* avatar) { this->avatar = avatar; }
protected:
	void PruneActions()
	{
		std::vector<unsigned int> to_delete;
		unsigned int action_index = -1;
		
		for (auto& a : actions)
		{
			//std::cout << "action" << action_index << std::endl;
			action_index++;
			
			if (a->IsActive())
				continue;
			std::cout << "action gone inactive" << std::endl;
			a->GetActor().ResetAction();
			std::cout << "actor reset" << std::endl;
			if (SkillAction* skill_action = dynamic_cast<SkillAction*>(a))
				for (auto& t : skill_action->GetTargets())
				{
					std::cout << "target" << std::endl;
					std::vector<Action*>& affected_by = 
						t->GetAffectedByMutable();
					
					unsigned int index = -1;
					for (unsigned int i = 0; i < affected_by.size(); i++)
						if (affected_by[i] == a)
						{
							index = i;
							break;
						}
					
					if (index == -1)
						continue;
						
					affected_by.erase(affected_by.begin() + index);
					std::cout << "target cleared" << std::endl;
				}
				
			to_delete.insert(to_delete.begin(), action_index);
			std::cout << "delete queued" << std::endl;
		}
		
		for (auto& i : to_delete)
		{
			Action* action = actions[i];
			std::cout << "action deleting" << std::endl;
			actions.erase(actions.begin() + i);
			delete action;
		}
	}
private:
	std::vector<Entity*> entities;
	std::vector<Action*> actions;
	Character* avatar = NULL;
	Endpoint& endpoint;
};

#endif
