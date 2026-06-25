# Phase - 1 (Planning and Stuffs)

- As of now I have a basic flow of the application. A C++ Database server which stores thing in-memory.
- Then we will have a CLI layer
- Then we will have a network layer which will let users interact with the server
- At last a JS SDK (and maybe in future python SDK as well) which acts as API layer that lets the application actually utilise the C++ database

# Phase - 2 (Basic DB)

- I have made Status.h, which is an enum class containing common errors, then a Response.h, which is a struct, which acts as a data type that has at max three things, status, message and data, so we can reuse it to send any response from any of our application
- I have also made Database.h, which has all the declarations and Database.cpp, which has the implementation of the Database, which is using a hashmap and returns Response for everything
- Modified the Response.h to use `std::variant` instead of `std::string`, which will let us store different types of data in the same Response struct
- Also we have now added more functions for string and also added for lists. Now every method related to setting and pushing has ttl property which is added to a custom DBEntry data type.
- We lazily check on all these operation if our key has expired by checking/verifying with the current time and if it does, we simply delete the key and never return anything
- Also for setting we are simply adding the current time to the ttl time set by the user
- Also we use somthing called std::holds_alternative, which is a function which helps in checking what kind of data is currently stored in our `std::variant`, it returns a boolean value, and after confirmation we use std::get to extract the that data