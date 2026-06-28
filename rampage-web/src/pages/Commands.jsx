import React from 'react';
import { Card, CardHeader, CardTitle, CardDescription, CardContent } from '../components/ui/card';

function Commands() {
    const commands = [
        { name: "PING", usage: "PING [message]", desc: "Returns PONG if no argument is provided, otherwise return a copy of the argument as a bulk. This command is often used to test if a connection is still alive, or to measure latency." },
        { name: "SET", usage: "SET key value [ttlSeconds]", desc: "Set key to hold the string value. If key already holds a value, it is overwritten, regardless of its type. Optionally accepts a TTL." },
        { name: "GET", usage: "GET key", desc: "Get the value of key. If the key does not exist the special value nil is returned. An error is returned if the value stored at key is not a string." },
        { name: "DEL", usage: "DEL key", desc: "Removes the specified key. A key is ignored if it does not exist." },
        { name: "TTL", usage: "TTL key", desc: "Returns the remaining time to live of a key that has a timeout." },
        { name: "EXPIRE", usage: "EXPIRE key seconds", desc: "Set a timeout on key. After the timeout has expired, the key will automatically be deleted." },
        { name: "APPEND", usage: "APPEND key value", desc: "If key already exists and is a string, this command appends the value at the end of the string. If key does not exist it is created and set as an empty string." },
        { name: "STRLEN", usage: "STRLEN key", desc: "Returns the length of the string value stored at key." },
        { name: "LPUSH", usage: "LPUSH key element [ttlSeconds]", desc: "Insert the specified value at the head of the list stored at key. If key does not exist, it is created." },
        { name: "RPUSH", usage: "RPUSH key element [ttlSeconds]", desc: "Insert the specified value at the tail of the list stored at key. If key does not exist, it is created." },
        { name: "LPOP", usage: "LPOP key", desc: "Removes and returns the first element of the list stored at key." },
        { name: "RPOP", usage: "RPOP key", desc: "Removes and returns the last element of the list stored at key." },
        { name: "LLEN", usage: "LLEN key", desc: "Returns the length of the list stored at key." },
        { name: "LINDEX", usage: "LINDEX key index", desc: "Returns the element at index index in the list stored at key." },
        { name: "LSET", usage: "LSET key index element", desc: "Sets the list element at index to element." },
        { name: "LRANGE", usage: "LRANGE key start stop", desc: "Returns the specified elements of the list stored at key. The offsets start and stop are zero-based indexes." },
    ];

    return (
        <div className="container mx-auto px-4 py-12 flex-1">
            <div className="mb-12 max-w-2xl">
                <h1 className="text-4xl md:text-5xl font-extrabold tracking-tight mb-4">
                    Supported Commands
                </h1>
                <p className="text-xl text-muted-foreground">
                    RAMpage currently supports the core Key-Value and List operations from the Redis catalog. All commands are fully RESP compatible.
                </p>
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 pb-12">
                {commands.map((cmd) => (
                    <Card key={cmd.name} className="flex flex-col border-primary/10 shadow-sm hover:shadow-md transition-shadow">
                        <CardHeader>
                            <div className="flex justify-between items-start mb-2">
                                <CardTitle className="text-2xl text-primary font-bold">{cmd.name}</CardTitle>
                                <span className="inline-flex items-center rounded-md border px-2.5 py-0.5 text-xs font-semibold transition-colors focus:outline-none focus:ring-2 focus:ring-ring focus:ring-offset-2 border-transparent bg-secondary text-secondary-foreground hover:bg-secondary/80 font-mono">O(1)</span>
                            </div>
                            <code className="text-sm bg-muted/50 p-2 rounded-md font-mono text-muted-foreground block border">
                                {cmd.usage}
                            </code>
                        </CardHeader>
                        <CardContent className="flex-1 text-muted-foreground">
                            {cmd.desc}
                        </CardContent>
                    </Card>
                ))}
            </div>
        </div>
    );
}

export default Commands;
