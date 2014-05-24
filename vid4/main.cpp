#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <map>
#include <algorithm>
#include <typeinfo>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <GL/glew.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#define TILE_SIZE 24

#include "common.h"
#include "comm.h"
#include "game.h"
#include "graphics.h"


Mesh* temp_gen_mesh()
{
	Vertex vertices[] = {
		Vertex(glm::vec3(-0.5,-0.5, 0.0),glm::vec2( 0.0, 0.0)),
		Vertex(glm::vec3(-0.5, 0.5, 0.0),glm::vec2( 0.0, 1.0)),
		Vertex(glm::vec3( 0.5, 0.5, 0.0),glm::vec2( 1.0, 1.0)),
		Vertex(glm::vec3( 0.5,-0.5, 0.0),glm::vec2( 1.0, 0.0)),
	};
	
	unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
		
	Mesh* ret = new Mesh(
		vertices, 
		sizeof(vertices) / sizeof(vertices[0]),
		indices,
		sizeof(indices) / sizeof(indices[0])
	);
	
	return ret;
}


bool temp_process_input(Character* avatar)
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
		if (e.type == SDL_QUIT)
			return false;
		else if (e.type == SDL_KEYDOWN && avatar)
		{
			Point next = avatar->GetPathEnd();
			
			switch (e.key.keysym.sym)
			{
			case 119: // Up
				next.y--;
				avatar->QueuePath(next);
				break;
			case 115: // Down
				next.y++;
				avatar->QueuePath(next);
				break;
			case 97: // Left
				next.x--;
				avatar->QueuePath(next);
				break;
			case 100: // Right
				next.x++;
				avatar->QueuePath(next);
				break;
			case 102: // F (aoe)
				break;
			default:
				std::cout << "Key code: " << e.key.keysym.sym << std::endl;
			}
		}
	return true;
}


class ServerSimulator
{
public:
	ServerSimulator(Endpoint* uplink) : uplink(uplink) {}
	
	void Update()
	{
		static unsigned int state = -1000;
		unsigned int now = Time::GetNow();
		
		if (state == -1000 && now - start > 1000)
		{
			IdentityMessage* msg = new IdentityMessage();
			
			msg->entity_id = 0;
			msg->name = "Kirtah";
			msg->skin = "yeti";
			msg->map = "Himalayas";
			msg->loc.x = -5;
			msg->loc.y = 0;
			
			uplink->Send(msg);
			state = 0;
		}
		else if (state == 0 && now - start > 1500)
		{
			EntityAppearMessage* msg = new EntityAppearMessage();
			
			msg->entity_id = 1;
			msg->name = "Zathril";
			msg->skin = "azlar";
			
			uplink->Send(msg);
			state = 3;
		}
		else if (state == 3 && now - start > 1700)
		{
			EntityActionMessage* msg = new EntityActionMessage();
			
			msg->entity_id = 1;
			msg->action_id = 3;
			msg->action_loc = Point(4,5);

			ActionAffectedDetails detail = ActionAffectedDetails();
			detail.entity_id = 1;
			detail.hp = 100;
			msg->affected.push_back(detail);
			
			uplink->Send(msg);
			state = 4;
		}
		else if (state == 4 && now - start > 2300)
		{
			EntityAppearMessage* msg = new EntityAppearMessage();
			
			msg->entity_id = 2;
			msg->name = "Yeti";
			msg->skin = "yeti";
			msg->loc.x = 4;
			msg->loc.y = -2;
			
			uplink->Send(msg);
			state = 5;
		}
		else if (state == 5 && now - start > 2500)
		{
			EntityMoveMessage* msg = new EntityMoveMessage();
			
			msg->entity_id = 1;
			msg->speed = 150;
			msg->path.push_back(Point(0,0));
			msg->path.push_back(Point(1,0));
			msg->path.push_back(Point(2,0));
			msg->path.push_back(Point(2,1));
			msg->path.push_back(Point(2,2));
			msg->path.push_back(Point(2,3));
			msg->path.push_back(Point(3,4));
			msg->path.push_back(Point(4,5));
			
			uplink->Send(msg);
			state = 10;
		}
		else if (state == 10 && now - start > 5000)
		{
			EntityActionMessage* msg = new EntityActionMessage();
			
			msg->entity_id = 1;
			msg->action_id = 5;
			msg->action_loc = Point(4,5);

			ActionAffectedDetails detail = ActionAffectedDetails();
			detail.entity_id = 2;
			detail.hp = 60;
			msg->affected.push_back(detail);
			
			uplink->Send(msg);
			state = 20;
		}
		else if (state == 20 && now - start > 6000)
		{
			EntityMoveMessage* msg = new EntityMoveMessage();
			
			msg->entity_id = 1;
			msg->speed = 175;
			msg->path.push_back(Point(4,4));
			msg->path.push_back(Point(4,3));
			msg->path.push_back(Point(3,2));
			msg->path.push_back(Point(2,1));
			msg->path.push_back(Point(1,1));
			
			uplink->Send(msg);
			state = 50;
		}
		else if (state == 50 && now - start > 6500)
		{
			EntityActionMessage* msg = new EntityActionMessage();
			
			msg->entity_id = 1;
			msg->action_id = 5;
			msg->action_loc = Point(4,5);

			ActionAffectedDetails detail = ActionAffectedDetails();
			detail.entity_id = 2;
			detail.hp = 20;
			msg->affected.push_back(detail);
		
			uplink->Send(msg);
			state = 100;
		}
		else if (state == 100 && now - start > 9000)
		{
			EntityDisappearMessage* msg = new EntityDisappearMessage();
			
			msg->entity_id = 1;
			
			uplink->Send(msg);
			state = 101;
		}
		else if (state == 101 && now - start > 10000)
		{
			EntityDisappearMessage* msg = new EntityDisappearMessage();
			
			msg->entity_id = 2;

			uplink->Send(msg);
			state = 500;
		}
		else if (state == 500 && now - start > 12000)
		{
			start = Time::GetNow();
			state = 0;
		}
	}
private:
	Endpoint* uplink;
	unsigned int start = Time::GetNow();
};


class ClientComm
{
public:
	ClientComm(GameEngine& game_engine, 
			   Endpoint& game_endpoint,
			   GraphicsEngine& graphics_engine,
			   AssetManager& asset_manager)
		: game_engine(game_engine),
		  game_endpoint(game_endpoint),
		  graphics_engine(graphics_engine),
		  asset_manager(asset_manager)
	{
	}
	
	void Update()
	{
		while (Message* msg = game_endpoint.Poll())
			Handle(msg);
	
		//while (Message* msg = graphics_endpoint.Poll())
			//Handle(msg);
	}
	
	void Handle(Message* msg)
	{
		if (EntityAppearMessage* m =
			dynamic_cast<EntityAppearMessage*>(msg))
		{
			std::cout << "EntityAppearMessage: " << m->name << std::endl;
			
			Character* character = new Character();
			character->SetID(m->entity_id);
			character->SetName(m->name);
			character->SetLoc(m->loc);
			
			game_engine.Register(character);
			
			YetiComponent* component = new YetiComponent(
				asset_manager, 
				*character,
				m->skin
			);
			
			graphics_engine.Register(component);
		}
		
		if (IdentityMessage* m = 
			dynamic_cast<IdentityMessage*>(msg))
		{
			std::cout << "IdentityMessage: " << m->name << std::endl;
			
			Entity* entity = game_engine.GetEntityByID(m->entity_id);
			Character* character = dynamic_cast<Character*>(entity);
			game_engine.SetAvatar(character);
		}
		
		if (EntityDisappearMessage* m = 
			dynamic_cast<EntityDisappearMessage*>(msg))
		{
			std::cout << "EntityDisappearMessage: " << m->entity_id << std::endl;
			
			Entity* entity = game_engine.GetEntityByID(m->entity_id);
			Component* component = graphics_engine.FindComponent(entity);

			graphics_engine.Deregister(component);
			game_engine.Deregister(entity);
			
			delete component;
			delete entity;
		}
		
		if (EntityMoveMessage* m =
			dynamic_cast<EntityMoveMessage*>(msg))
		{
			std::cout << "EntityMoveMessage: " << m->entity_id << std::endl;
			
			Entity* entity = game_engine.GetEntityByID(m->entity_id);
			Character* character = dynamic_cast<Character*>(entity);
			if (!character)
				throw std::runtime_error("Failed to cast Entity to Character.");
			
			character->ClearPath();
			
			for (auto& p : m->path)
				character->QueuePath(p);
				
			character->SetSpeed(m->speed);
		}
		
		if (EntityActionMessage* m =
			dynamic_cast<EntityActionMessage*>(msg))
		{
			std::cout << "EntityActionMessage: " << m->entity_id << std::endl;
			
			Entity* entity = game_engine.GetEntityByID(m->entity_id);
			Character* character = dynamic_cast<Character*>(entity);
			
			Skill skill;
			
			SkillAction* action = new SkillAction(*character, skill);
			
			for (auto& t : m->affected)
			{
				Entity* t_ent = game_engine.GetEntityByID(t.entity_id);
				Character* t_char = dynamic_cast<Character*>(t_ent);
				
				t_char->SetHP(t.hp);
				
				action->GetTargetsMutable().push_back(t_char);
			}
			
			game_engine.Register(*action);
		}
		
		delete msg;
	}
private:
	GameEngine& game_engine;
	GraphicsEngine& graphics_engine;
	
	Endpoint& game_endpoint;
	
	AssetManager& asset_manager;
};


int main()
{
	Router router;	

	Endpoint& uplink = router.Register(ADDR_UPLINK);	
	Endpoint& game_endpoint = router.Register(ADDR_GAME_ENGINE);

	GameEngine game_engine(game_endpoint);
	GraphicsEngine graphics_engine(game_engine);
	
	AssetManager asset_manager;
	asset_manager.GetTexture("yeti.png")->SetOffset(Point(0,35));
	asset_manager.GetTexture("azlar.png")->SetOffset(Point(0,28));
	asset_manager.RegisterMesh("square", temp_gen_mesh());
		
	ServerSimulator server_sim(&uplink);
	
	ClientComm client_comm(
		game_engine, 
		game_endpoint, 
		graphics_engine, 
		asset_manager
	);
	
	while (true)
	{
		server_sim.Update();
		client_comm.Update();
		
		if (!temp_process_input(game_engine.GetAvatar()))
			break;
		
		router.Dispatch();
		game_engine.Update();
		graphics_engine.Draw();
	}
	
	return 0;
}
