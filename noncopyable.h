//
// Created by zelin on 2022/5/11.
//

#ifndef EVENT_MANAGER_NONCOPYABLE_H
#define EVENT_MANAGER_NONCOPYABLE_H

struct noncopyable {
    noncopyable() = default;
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator= (const noncopyable&) = delete;
};

struct nonmovable {
    nonmovable() = default;
    nonmovable(nonmovable&&) = delete;
    nonmovable& operator= (nonmovable&&) = delete;
};

#endif //EVENT_MANAGER_NONCOPYABLE_H
