#ifndef COMM_H
#define COMM_H

#include <map>
#include <queue>
#include <stdexcept>

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>

#include "common.h"


enum COMM_ADDRESSES {
    ADDR_UPLINK,
    ADDR_GAME_ENGINE
};


enum ENTITY_ACTION_TYPE {
    TARGET,
    AOE
};


class Message
{
public:
    virtual ~Message() {}

    virtual int GetDestination() = 0;
};


class EntityMessage : public Message
{
public:
    unsigned int entity_id;

    virtual int GetDestination() { return ADDR_GAME_ENGINE; }
};


class EntityAppearMessage : public EntityMessage
{
public:
    std::string name;
    std::string skin;
    Point loc;
};


class IdentityMessage : public EntityAppearMessage
{
public:
    std::string map;
};


class EntityDisappearMessage : public EntityMessage {};


class EntityMoveMessage : public EntityMessage
{
public:
    unsigned int speed;
    std::vector<Point> path;
};


struct ActionAffectedDetails
{
    int entity_id;
    int hp;
};


class EntityActionMessage : public EntityMessage
{
public:
    unsigned int action_id;
    unsigned int skill_id;
    Point action_loc;
    std::vector<ActionAffectedDetails> affected;
};


// Adapted from http://www.justsoftwaresolutions.co.uk/threading/ ->
//           -> implementing-a-thread-safe-queue-using-condition-variables.html
class MessageQueue
{
private:
    std::queue<Message*> the_queue;
    mutable boost::mutex the_mutex;
    boost::condition_variable the_condition_variable;
public:
    void push(Message* data)
    {
        boost::mutex::scoped_lock lock(the_mutex);
        the_queue.push(data);
        lock.unlock();
        the_condition_variable.notify_one();
    }

    bool empty() const
    {
        boost::mutex::scoped_lock lock(the_mutex);
        return the_queue.empty();
    }

    Message* try_pop()
    {
        boost::mutex::scoped_lock lock(the_mutex);

        if(the_queue.empty())
            return NULL;

        Message* ret = the_queue.front();
        the_queue.pop();
        return ret;
    }

    Message* wait_and_pop()
    {
        boost::mutex::scoped_lock lock(the_mutex);
        while(the_queue.empty())
            the_condition_variable.wait(lock);

        Message* ret = the_queue.front();
        the_queue.pop();
        return ret;
    }

};


class Endpoint
{
public:
    Endpoint(
        MessageQueue& input,
        MessageQueue& output
    )
        : input(input),
          output(output)
    { }

    void Send(Message* message)
    {
        output.push(message);
    }

    Message* Poll()
    {
        return input.try_pop();
    }
protected:
private:
    MessageQueue& input;
    MessageQueue& output;
};


class Router
{
private:
    struct EndpointData
    {
        MessageQueue* in;
        MessageQueue* out;

        Endpoint* local;
        Endpoint* remote;
    };

public:
    Router()
    {
    }

    virtual ~Router()
    {
        for (auto& e : endpoints)
        {
            delete e.second.in;
            delete e.second.out;
            delete e.second.local;
            delete e.second.remote;
        }
    }

    virtual Endpoint& Register(int address)
    {
        EndpointData d;

        d.in = new MessageQueue();
        d.out = new MessageQueue();

        d.local = new Endpoint(*d.in, *d.out);
        d.remote = new Endpoint(*d.out, *d.in);

        endpoints.insert(std::pair<int,EndpointData>(address, d));

        return *d.remote;
    }

    virtual void Dispatch()
    {
        for (auto& e : endpoints)
        {
            Message* message = e.second.local->Poll();

            if (!message)
                continue;

            if (e.first != ADDR_UPLINK)
            {
                endpoints[ADDR_UPLINK].local->Send(message);
                continue;
            }

            endpoints[message->GetDestination()].local->Send(message);
        }
    }
protected:
private:
    EndpointData uplink;
    std::map<int,EndpointData> endpoints;
};

#endif
