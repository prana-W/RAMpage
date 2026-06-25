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

### 3. `main.cpp` (The Glue)
This is where everything is wired together into a running application.
1. **Setup:** It creates exactly one instance of the `Database` and exactly one instance of the `CommandRegistry`.
2. **Registration:** It calls `registerStringCommands(registry)` and `registerListCommands(registry)`. This passes the empty registry into our modules, where it gets fully "loaded up" with all the lambda functions.
3. **The Loop:** It starts the infinite `while (std::getline(...))` loop, constantly waiting for your input.
4. **Execution:** When you press Enter, it hands the raw string you typed directly to `registry.execute(db, line)`. The registry finds the right lambda, passes the `db` into it so the database can be modified, and spits out a resulting string.
5. **Output:** `main.cpp` passes that result through `prettyPrint` and prints it to your terminal!

This architecture makes it incredibly easy to add new features. If you wanted to add Hash maps tomorrow, you wouldn't have to touch `main.cpp`'s logic at all—you'd just make a `HashCommands.cpp` file, register it, and it would instantly work!