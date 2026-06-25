# Phase - 1 (Planning and Stuffs)

- As of now I have a basic flow of the application. A C++ Database server which stores thing in-memory.
- Then we will have a CLI layer
- Then we will have a network layer which will let users interact with the server
- At last a JS SDK (and maybe in future python SDK as well) which acts as API layer that lets the application actually utilise the C++ database

# Phase - 2 (Basic DB)

- BTW read about ODR (One definition rule), declare every thing in one file with '.h' and use that everywehre we want to actually use the stuff from that file and then only compile that files .cpp once, else if we include the cpp file directly evrywhere, it will make the same files copy everywhere and resulting in erro because of multiple definitions of the same functions/variables etc.
- I have made Status.h, which is an enum class containing common errors, then a Response.h, which is a struct, which acts as a data type that has at max three things, status, message and data, so we can reuse it to send any response from any of our application
- I have also made Database.h, which has all the declarations and Database.cpp, which has the implementation of the Database, which is using a hashmap and returns Response for everything
- Modified the Response.h to use `std::variant` instead of `std::string`, which will let us store different types of data in the same Response struct
- Also we have now added more functions for string and also added for lists. Now every method related to setting and pushing has ttl property which is added to a custom DBEntry data type.
- We lazily check on all these operation if our key has expired by checking/verifying with the current time and if it does, we simply delete the key and never return anything
- Also for setting we are simply adding the current time to the ttl time set by the user
- Also we use somthing called std::holds_alternative, which is a function which helps in checking what kind of data is currently stored in our `std::variant`, it returns a boolean value, and after confirmation we use std::get to extract the that data
- I have setup CMake, so now just go to build folder and run `cmake ..` then `make`, it will build the project and create an executable file named `rampage_cli` which is our CLI application, we can also run `db_test` for test purposes
- Now at this point we have basic functions/methods with us that store data in the unordered_map of key as string and val as either string or deque. We also have TTL support now, with lazy deletion as of now (have to make some auto-deletion after few second using multithreading later)

# Phase - 3 (rampage_cli)

- Now we want to interact with the DB thorough our CLI and for that we have to register some commands and based on the command, we can do the necessary thing
- Ran command: `./rampage_cli`
Viewed main.cpp:9-27

Here is a clear breakdown of how the architecture we've built flows from end to end!

### 1. `CommandRegistry` (The Dispatcher)
Think of the Command Registry as a telephone switchboard operator. 
- **`CommandRegistry.h`** defines a hash map (`std::unordered_map`) where the keys are command names (like `"SET"`, `"LPUSH"`) and the values are C++ functions (`std::function`) that know how to execute that command.
- **`CommandRegistry.cpp`** does the heavy lifting when you type into the CLI. When you type `set one 3`, the `execute()` function kicks in. It calls `tokenize` to split the string into `["set", "one", "3"]`. It takes the first word (`"set"`), capitalizes it to `"SET"`, looks it up in the hash map, and hands the rest of the arguments (`["one", "3"]`) over to the specific function registered for `"SET"`.

### 2. `StringCommands.cpp` & `ListCommands.cpp` (The Modules)
If we put every single command inside `main.cpp`, it would quickly turn into a 10,000-line unreadable file filled with `if-else` statements. 

To solve this, we modularized the commands. 
- These files act as "plugins." They each expose a single setup function (like `registerStringCommands`). 
- Inside these setup functions, we define **lambdas** (anonymous, inline C++ functions) and map them to their string names (e.g., `reg.registerCommand("SET", [](...) { ... })`).
- These lambdas act as the "bridge" between the CLI and the Database. They take the raw string arrays from the CLI, do safety checks (like "did the user provide enough arguments?"), convert strings to integers if necessary, call the actual `Database` method, and then format the `Response` struct into the final string that gets printed to the screen.

### 3. `rampage_cli.cpp` (The Glue)
This is where everything is wired together into a running application.
1. **Setup:** It creates exactly one instance of the `Database` and exactly one instance of the `CommandRegistry`.
2. **Registration:** It calls `registerStringCommands(registry)` and `registerListCommands(registry)`. This passes the empty registry into our modules, where it gets fully "loaded up" with all the lambda functions.
3. **The Loop:** It starts the infinite `while (std::getline(...))` loop, constantly waiting for your input.
4. **Execution:** When you press Enter, it hands the raw string you typed directly to `registry.execute(db, line)`. The registry finds the right lambda, passes the `db` into it so the database can be modified, and spits out a resulting string.
5. **Output:** `rampage_cli.cpp` passes that result through `prettyPrint` and prints it to your terminal!

This architecture makes it incredibly easy to add new features. If you wanted to add Hash maps tomorrow, you wouldn't have to touch `rampage_cli.cpp`'s logic at all—you'd just make a `HashCommands.cpp` file, register it, and it would instantly work!

## Phase - 4 (TCP Server)

- I am trying to have a similar architecture to redis, that is my db server will allow multiple clients to connect to it at the same time, all of them can access the data present in the db.
- But instead of spawning threads per client, I am using a single main thread with epoll, which is a Linux-specific API for efficient I/O multiplexing (One thread/process watches many connections at a time), it allows a single thread to monitor multiple file descriptors (like sockets) at the same time and only react when one of them is actually ready to read/write, which is way more efficient than spawning a thread per client
- We have Server.cpp, Server.h and main.cpp for the entire server setup. Server.h declares everything, Server.cpp has the actual implementation, and main.cpp is the entry point which sets up the db, registers all commands and starts the server
- We have a `start()` method in Server.cpp which sets up the socket (bind, listen), creates the epoll instance, and then enters the event loop using `epoll_wait()` which blocks until any client (or the server socket itself) has data ready
- When epoll says the server socket is ready, it means a new client wants to connect, so we `accept()` it, set it to non-blocking mode and register it with epoll too. We also create a personal buffer (just an empty string for now) for that client in a map
- When epoll says a client socket is ready, it means that client has sent some data, so we `recv()` it and append it to that client's personal buffer
- We also have a `processClientBuffer()` method which processes all the complete commands (lines ending with `\n`) sitting in the client's buffer, one by one sequentially. TCP is a stream protocol so data might arrive in chunks, so keeping a per-client buffer and only executing when we see a `\n` is the right approach
- Since everything runs in a single thread, there is no concept of race conditions at all!
- Improve error handling. Now for any error/non-success, the redis server sends a ERR:... and for success SUCC:...., so by proper parsing the start of the strings, we can differentiate between success and error, and for success case we have further different return types based on the command.

# Phase - 5 (Nodejs SDK)

- Implement an ESM based SDK
- It is made to exaclty feel like the Redis SDK
- Users can call methods like client.get('name)
- RampageClient formats strings
- It is added to the RampageConnection class and request is added to a queue and command is added to TCP socket
- Data comming from the server is in chunks and connection maintiains a string buffer and appends incoming chunks till it sees a \n
- After a \n is received, then that command is resolved and popoed from the queue
- Other error handling like number handling, no key handling, ERR, SUCC handling is done
- We have Auto reconnect, exponential backoff, event emitter (connect, error, close, reconnecting), in-flight draining in RampageConnection Class
- Implementing a custom Rampage Error class for handling specific errors