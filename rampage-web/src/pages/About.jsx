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

                    <h2 className="text-2xl font-bold text-foreground mt-10 mb-4">Core Architecture</h2>
                    <ul className="list-disc pl-6 space-y-2">
                        <li>
                            <strong>Event-Driven I/O:</strong> Built on <code>epoll</code>, the server runs on a single thread and uses non-blocking sockets. This eliminates the need for expensive context switches and mutex locks on the hot path.
                        </li>
                        <li>
                            <strong>Zero-Copy Networking:</strong> Commands like <code>LRANGE</code> use advanced C++ features like <code>std::string_view</code> to pipe data straight from the internal memory data structures to the TCP socket without a single memory reallocation.
                        </li>
                        <li>
                            <strong>AOF Persistence:</strong> Data durability is guaranteed through an Append-Only File (AOF). To prevent disk I/O from blocking the server, the PersistenceManager uses a lock-free background flusher thread to safely persist commands to disk.
                        </li>
                        <li>
                            <strong>Native RESP Protocol:</strong> RAMpage speaks the REdis Serialization Protocol natively. This means any standard Redis client or SDK (Node.js, Go, Python) can interact with RAMpage perfectly out-of-the-box.
                        </li>
                    </ul>

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
