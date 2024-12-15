#include "monitor.h"
#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <iomanip>
#include <vector>

struct Item{
    std::string producer_name;
    int produced_number;
};

class Buffer: public Monitor{
public:
    Buffer(const std::string& name, size_t max_size);
    void insert(Item &item);
    Item remove();
private:
    std::queue<Item> _content;
    Condition _empty, _full;
    std::string _name;
    size_t _max_size;
    // Semaphore s - defined in Monitor plays role of _mutex
};

class Producer{
public:
    Producer(const std::string& name) : _name(name){}
    void run();
    void produce_item();
    void addBuffer(Buffer* buffer);
private:
    std::vector<Buffer*> _buffers{};
    std::string _name;
    Item _item{};
};

class Consumer{
public:
    Consumer(const std::string& name) : _name(name){}
    void run();
    void consume_item();
    void addBuffer(Buffer* buffer);
private:
    std::vector<Buffer*> _buffers {};
    std::string _name;
    Item _item{};
};


// GLOBAL CONSOLE MUTEX SEMAPHORE//
Semaphore console(1);
// this could also be implemented as Monitor
// but for sake of simplicity (since we only need mutex) it is a semaphore


// MAIN //
int main(){
    Buffer b1("1", 6);
    Buffer b2("2", 2);
    Buffer b3("3", 1);
    Buffer b4("4", 9);

    Producer p1("pA");
    p1.addBuffer(&b1);
    Producer p2("pB");
    p2.addBuffer(&b1);
    p2.addBuffer(&b2);
    p2.addBuffer(&b3);
    p2.addBuffer(&b4);
    Producer p3("pC");
    p3.addBuffer(&b4);

    Consumer c1("cA");
    c1.addBuffer(&b1);
    Consumer c2("cB");
    c2.addBuffer(&b2);
    Consumer c3("cC");
    c3.addBuffer(&b3);
    Consumer c4("cD");
    c4.addBuffer(&b4);

    std::thread producer1_t(&Producer::run, &p1);
    std::thread producer2_t(&Producer::run, &p2);
    std::thread producer3_t(&Producer::run, &p3);

    std::thread consumer1_t(&Consumer::run, &c1);
    std::thread consumer2_t(&Consumer::run, &c2);
    std::thread consumer3_t(&Consumer::run, &c3);
    std::thread consumer4_t(&Consumer::run, &c4);

    producer1_t.detach();
    producer2_t.detach();
    producer3_t.detach();

    consumer1_t.detach();
    consumer2_t.detach();
    consumer3_t.detach();
    consumer4_t.detach();
    
    while(true)
    {
        //keep main alive
    }
    return 0;
}


// Functions definitions //

Buffer::Buffer(const std::string &name, size_t max_size) :
    _content(), _empty(), _full(), _name(name), _max_size(max_size)
{
}

void Buffer::insert(Item &item)
{
    enter();
    if (_content.size() == _max_size)
        wait(_full);
    _content.push(std::move(item));
    console.p();
    std::cout<<"Buffer: "<<std::quoted(_name)<<" got item: "<<_content.back().produced_number<<" from producer: "<<std::quoted(_content.back().producer_name)<<std::endl;
    console.v();
    if (_content.size() == 1)
        signal(_empty);
    leave();
}

Item Buffer::remove()
{
    enter();
    if (_content.empty())
        wait(_empty);
    Item i = _content.front();
    _content.pop();
    console.p();
    std::cout<<"Buffer: "<<std::quoted(_name)<<" got item removed: "<<i.produced_number<<" from producer: "<<std::quoted(i.producer_name)<<std::endl;
    console.v();
    if (_content.size() == _max_size - 1)
        signal(_full);
    leave();
    return i;
}

void Producer::run()
{
    while (true){
        for (auto * buffer : _buffers){
            produce_item();
            buffer->insert(_item);
            // sleep(1);
        }
    }

}

void Producer::produce_item()
{
    _item = Item();
    _item.producer_name = _name;
    _item.produced_number = rand();
    console.p();
    std::cout<<"\t\tProducer: "<<std::quoted(_name)<<" produced item: "<<_item.produced_number<<std::endl;
    console.v();
}

void Producer::addBuffer(Buffer *buffer)
{
    _buffers.push_back(buffer);
}

void Consumer::run()
{
    while (true){
        for (auto * buffer : _buffers){
            _item = buffer->remove();
            consume_item();
            // sleep(1);
        }
    }
}

void Consumer::consume_item()
{
    console.p();
    std::cout<<"\t\tConsumer: "<<std::quoted(_name)<<" consumed item: "<<_item.produced_number<<std::endl;
    console.v();
}

void Consumer::addBuffer(Buffer *buffer)
{
    _buffers.push_back(buffer);
}
