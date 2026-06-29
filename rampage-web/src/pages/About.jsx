import React from 'react';
import { Card, CardHeader, CardTitle, CardContent } from '../components/ui/card';
import { Github } from 'lucide-react';
import { Button } from '../components/ui/button';

function About() {
    return (
        <div className="container mx-auto px-4 py-12 flex-1">
            <div className="max-w-3xl mx-auto">
                <h1 className="text-4xl md:text-5xl font-extrabold tracking-tight mb-6">
                    About RAMpage
                </h1>
                
                <div className="prose prose-slate dark:prose-invert max-w-none text-muted-foreground text-lg leading-relaxed space-y-6">
                    <p>
                        <strong>RAMpage</strong> is a Redis-inspired, high-performance in-memory database server implemented entirely from scratch in C++.
                        It rampages through reads and writes with ultra-low latency, leveraging modern Linux I/O via <code>epoll</code>.
                    </p>
                    
                    <h2 className="text-2xl font-bold text-foreground mt-10 mb-4">Why Build Another Database?</h2>
                    <p>
                        The primary goal of RAMpage was to deeply understand and master high-performance networking, low-level memory management, and database architectures. By bypassing the standard C++ networking abstractions and building directly on top of Linux kernel syscalls (<code>epoll</code>), RAMpage achieves phenomenal throughput and latency.
                    </p>

                    <h2 className="text-2xl font-bold text-foreground mt-10 mb-4">Under the Hood: How it Works</h2>
                    
                    <div className="space-y-8">
                        <div>
                            <h3 className="text-xl font-semibold text-primary mb-2">1. Event-Driven I/O Multiplexing</h3>
                            <p>
                                RAMpage relies heavily on <strong>Linux epoll</strong>. Instead of spawning an expensive operating system thread for every connected client, the server operates on a single main thread. It watches hundreds of non-blocking TCP sockets simultaneously and only wakes up to process I/O when a socket is actually ready. Because the hot path is entirely single-threaded, there are <strong>zero mutexes or race conditions</strong> when accessing the core hash map.
                            </p>
                        </div>

                        <div>
                            <h3 className="text-xl font-semibold text-primary mb-2">2. Modular Command Registry</h3>
                            <p>
                                The architecture avoids massive <code>if-else</code> blocks. A central <code>CommandRegistry</code> maintains an <code>std::unordered_map</code> that maps command strings (e.g. "SET", "LRANGE") directly to C++ lambda functions. This plugin-like architecture makes adding new commands trivial without ever touching the core networking loop.
                            </p>
                        </div>

                        <div>
                            <h3 className="text-xl font-semibold text-primary mb-2">3. Extreme Micro-Optimizations</h3>
                            <p>
                                After profiling, RAMpage was aggressively optimized to eliminate hidden C++ overheads, allowing it to outperform Redis on heavy workloads like <code>LRANGE</code>:
                            </p>
                            <ul className="list-disc pl-6 mt-2 space-y-1">
                                <li><strong>Zero-Copy:</strong> Uses <code>std::string_view</code> to pipe data straight from the internal <code>std::deque</code> into the TCP socket without heap allocations.</li>
                                <li><strong>Direct RESP Builds:</strong> Command handlers bypass intermediate serialization by constructing raw RESP wire bytes directly.</li>
                                <li><strong>No std::to_string:</strong> Uses C++17's <code>&lt;charconv&gt;</code> to serialize numbers without dynamic memory allocation.</li>
                                <li><strong>Batched Syscalls:</strong> Implements greedy pipelining to batch dozens of commands into a single `send()` syscall.</li>
                            </ul>
                        </div>

                        <div>
                            <h3 className="text-xl font-semibold text-primary mb-2">4. Non-Blocking AOF Persistence</h3>
                            <p>
                                Data durability is guaranteed through an Append-Only File (AOF). To prevent disk I/O from blocking the blazing-fast main thread, the <code>PersistenceManager</code> uses the classic Producer-Consumer pattern. 
                            </p>
                            <p className="mt-2">
                                The main thread pushes commands to a shared queue under a microsecond <code>std::mutex</code> lock and rings a <code>std::condition_variable</code>. A dedicated background flusher thread wakes up, performs an <strong>O(1) pointer swap</strong> to steal the queue contents into a local variable, releases the lock instantly, and handles the slow disk write privately. The main thread is never blocked waiting for the disk!
                            </p>
                        </div>

                        <div>
                            <h3 className="text-xl font-semibold text-primary mb-2">5. Real-Time Pub/Sub Engine</h3>
                            <p>
                                RAMpage fully supports Redis-compatible Publish/Subscribe messaging. This required completely breaking the standard Request-Response paradigm. The server maintains a strict "Subscriber Mode" context for clients, mapping exact channels and glob-style patterns directly to socket file descriptors.
                            </p>
                            <p className="mt-2">
                                When a <code>PUBLISH</code> command is received, the <code>PubSubManager</code> performs a zero-blocking fan-out by proactively formatting the payload as a RESP push array and calling the OS-level <code>send()</code> directly on the publisher's thread cycle. This ensures messages are instantly broadcast to all subscribers without relying on slow background queues, while explicitly bypassing the AOF persistence layer to keep ephemeral traffic out of the data log.
                            </p>
                        </div>

                        <div>
                            <h3 className="text-xl font-semibold text-primary mb-2">6. Native RESP Protocol</h3>
                            <p>
                                RAMpage features a custom RESP (REdis Serialization Protocol) parser with partial frame buffering. This allows any standard Redis client or SDK (Node.js, Go, Python) or tool like <code>redis-benchmark</code> to interact with RAMpage perfectly out-of-the-box.
                            </p>
                        </div>
                    </div>

                    <div className="mt-12 pt-8 border-t flex flex-col sm:flex-row items-center justify-between gap-4">
                        <p className="text-sm font-medium">
                            Ready to dive into the code?
                        </p>
                        <Button asChild>
                            <a href="https://github.com/prana-w/rampage" target="_blank" rel="noreferrer">
                                <Github className="mr-2 h-4 w-4" /> View Source on GitHub
                            </a>
                        </Button>
                    </div>
                </div>
            </div>
        </div>
    );
}

export default About;
