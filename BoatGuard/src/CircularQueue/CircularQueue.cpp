#include "CircularQueue/CircularQueue.h"
#include <cstdint>

template <class T>
CircularQueue<T>::CircularQueue(): head(0), tail(0) {
    for(int i = 0; i < size; i++) {
        array[i] = -1;
    }
}

template <class T>
void CircularQueue<T>::push(T elem) {
    array[head] = elem;          
    tail = head;
    head++;
    if(head >= size) {
        head = 0;
    } 
}

template <class T>
int CircularQueue<T>::getIndex(T elem) {
    for(int i = 0; i < size; i++) {
        if(elem == array[i]) {
            return i;
        }
    }
    return -1;
}

template class CircularQueue<uint8_t>;
template class CircularQueue<uint16_t>;

