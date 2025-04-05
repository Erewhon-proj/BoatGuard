#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

template<class T>
class CircularQueue {
public:
    CircularQueue();
    void push(T elem);
    int getIndex(T elem);

private:
    int size = 10;
    T array[10];
    int head;
    int tail;
};

#endif
