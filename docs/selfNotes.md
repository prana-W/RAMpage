# Phase - 1 (Planning and Stuffs)

- As of now I have a basic flow of the application. A C++ Database server which stores thing in-memory.
- Then we will have a CLI layer
- Then we will have a network layer which will let users interact with the server
- At last a JS SDK (and maybe in future python SDK as well) which acts as API layer that lets the application actually utilise the C++ database

# Phase - 2 (Basic DB)

- I have made Status.h, which is an enum class containing common errors, then a Response.h, which is a struct, which acts as a data type that has at max three things, status, message and data, so we can reuse it to send any response from any of our application
- I have also made Database.h, which has all the declarations and Database.cpp, which has the implementation of the Database, which is using a hashmap and returns Response for everything