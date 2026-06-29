import React, { useState } from 'react';
import { Card, CardHeader, CardTitle, CardDescription, CardContent } from '../components/ui/card';
import { Copy, Check } from 'lucide-react';
import { Button } from '../components/ui/button';

function Commands() {
    const commandCategories = [
        {
            title: "General",
            id: "general",
            commands: [
                { name: "PING", usage: "PING [message]", desc: "Returns PONG if no argument is provided, otherwise return a copy of the argument as a bulk. This command is often used to test if a connection is still alive, or to measure latency." },
            ]
        },
        {
            title: "Strings",
            id: "strings",
            commands: [
                { name: "SET", usage: "SET key value [ttlSeconds]", desc: "Set key to hold the string value. If key already holds a value, it is overwritten, regardless of its type. Optionally accepts a TTL." },
                { name: "GET", usage: "GET key", desc: "Get the value of key. If the key does not exist the special value nil is returned. An error is returned if the value stored at key is not a string." },
                { name: "DEL", usage: "DEL key", desc: "Removes the specified key. A key is ignored if it does not exist." },
                { name: "TTL", usage: "TTL key", desc: "Returns the remaining time to live of a key that has a timeout." },
                { name: "EXPIRE", usage: "EXPIRE key seconds", desc: "Set a timeout on key. After the timeout has expired, the key will automatically be deleted." },
                { name: "APPEND", usage: "APPEND key value", desc: "If key already exists and is a string, this command appends the value at the end of the string. If key does not exist it is created and set as an empty string." },
                { name: "STRLEN", usage: "STRLEN key", desc: "Returns the length of the string value stored at key." },
            ]
        },
        {
            title: "Lists",
            id: "lists",
            commands: [
                { name: "LPUSH", usage: "LPUSH key element [ttlSeconds]", desc: "Insert the specified value at the head of the list stored at key. If key does not exist, it is created." },
                { name: "RPUSH", usage: "RPUSH key element [ttlSeconds]", desc: "Insert the specified value at the tail of the list stored at key. If key does not exist, it is created." },
                { name: "LPOP", usage: "LPOP key", desc: "Removes and returns the first element of the list stored at key." },
                { name: "RPOP", usage: "RPOP key", desc: "Removes and returns the last element of the list stored at key." },
                { name: "LLEN", usage: "LLEN key", desc: "Returns the length of the list stored at key." },
                { name: "LINDEX", usage: "LINDEX key index", desc: "Returns the element at index index in the list stored at key." },
                { name: "LSET", usage: "LSET key index element", desc: "Sets the list element at index to element." },
                { name: "LRANGE", usage: "LRANGE key start stop", desc: "Returns the specified elements of the list stored at key. The offsets start and stop are zero-based indexes." },
            ]
        },
        {
            title: "Pub/Sub",
            id: "pubsub",
            commands: [
                { name: "SUBSCRIBE", usage: "SUBSCRIBE channel [channel ...]", desc: "Subscribes the client to the specified channels. Once subscribed, the client enters subscriber mode." },
                { name: "UNSUBSCRIBE", usage: "UNSUBSCRIBE [channel [channel ...]]", desc: "Unsubscribes the client from the given channels, or from all of them if none is given." },
                { name: "PSUBSCRIBE", usage: "PSUBSCRIBE pattern [pattern ...]", desc: "Subscribes the client to the given patterns." },
                { name: "PUNSUBSCRIBE", usage: "PUNSUBSCRIBE [pattern [pattern ...]]", desc: "Unsubscribes the client from the given patterns, or from all of them if none is given." },
                { name: "PUBLISH", usage: "PUBLISH channel message", desc: "Posts a message to the given channel, returning the number of receivers." },
                { name: "PUBSUB", usage: "PUBSUB subcommand [args]", desc: "Introspection command that allows to inspect the state of the Pub/Sub subsystem." },
            ]
        }
    ];

    const [copiedCmd, setCopiedCmd] = useState(null);

    const handleCopy = (text, name) => {
        navigator.clipboard.writeText(text);
        setCopiedCmd(name);
        setTimeout(() => setCopiedCmd(null), 2000);
    };

    const scrollToSection = (id) => {
        const element = document.getElementById(id);
        if (element) {
            // Offset for potential sticky header
            const y = element.getBoundingClientRect().top + window.scrollY - 100;
            window.scrollTo({ top: y, behavior: 'smooth' });
        }
    };

    return (
        <div className="container mx-auto px-4 py-12 flex-1">
            <div className="mb-8 max-w-2xl">
                <h1 className="text-4xl md:text-5xl font-extrabold tracking-tight mb-4">
                    Supported Commands
                </h1>
                <p className="text-xl text-muted-foreground">
                    RAMpage currently supports the core Key-Value, List, and Pub/Sub operations from the Redis catalog. All commands are fully RESP compatible.
                </p>
            </div>

            {/* Sub Nav-Bar */}
            <div className="flex flex-wrap gap-2 mb-12 p-1 bg-muted/30 rounded-lg max-w-fit">
                {commandCategories.map((category) => (
                    <Button 
                        key={category.id} 
                        variant="ghost" 
                        className="hover:bg-background"
                        onClick={() => scrollToSection(category.id)}
                    >
                        {category.title}
                    </Button>
                ))}
            </div>

            <div className="space-y-16 pb-12">
                {commandCategories.map((category) => (
                    <div key={category.id} id={category.id} className="scroll-mt-24">
                        <div className="border-b pb-4 mb-8">
                            <h2 className="text-3xl font-bold tracking-tight">{category.title}</h2>
                        </div>
                        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
                            {category.commands.map((cmd) => (
                                <Card key={cmd.name} className="flex flex-col border-primary/10 shadow-sm hover:shadow-md transition-shadow">
                                    <CardHeader>
                                        <div className="flex justify-between items-start mb-2">
                                            <CardTitle className="text-2xl text-primary font-bold">{cmd.name}</CardTitle>
                                        </div>
                                        <div className="relative group">
                                            <code className="text-sm bg-muted/50 p-2 pr-10 rounded-md font-mono text-muted-foreground block border">
                                                {cmd.usage}
                                            </code>
                                            <Button 
                                                variant="ghost" 
                                                size="icon" 
                                                className="absolute right-1 top-1 h-6 w-6 opacity-0 group-hover:opacity-100 transition-opacity"
                                                onClick={() => handleCopy(cmd.usage, cmd.name)}
                                            >
                                                {copiedCmd === cmd.name ? <Check className="h-3 w-3 text-primary" /> : <Copy className="h-3 w-3" />}
                                            </Button>
                                        </div>
                                    </CardHeader>
                                    <CardContent className="flex-1 text-muted-foreground">
                                        {cmd.desc}
                                    </CardContent>
                                </Card>
                            ))}
                        </div>
                    </div>
                ))}
            </div>
        </div>
    );
}

export default Commands;
