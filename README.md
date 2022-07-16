## event manager
### platform
- OS: linux

A simple event manager with a thread pool, register and trigger the events.

c++14 required.


``` c++
event_pool ep;
ep.register_callback("a", [](int a) {printf("%d\n");});
ep.register_callback("a", 1);
```

 more exmples, see the test files